#include <signal.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "AtomDBProxy.h"
#include "AtomDBSingleton.h"
#include "MettaParser.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "TestConfig.h"
#include "commons/atoms/MettaParserActions.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"
#include "MockAnimalsData.h"

using namespace std;
using namespace atomdb;
using namespace metta;
using namespace atoms;

using namespace atomdb_broker;
using namespace service_bus;

void ctrl_c_handler(int) {
    cout << "Stopping tests_db_loader..." << endl;
    cout << "Done." << endl;
    exit(0);
}

vector<string> add_atoms_via_atomdb(vector<Atom*> atoms, bool throw_if_exists, bool is_transactional) {
    auto db = AtomDBSingleton::get_instance();
    return db->add_atoms(atoms, throw_if_exists, is_transactional);
}

int main(int argc, char* argv[]) {
    TestConfig::load_environment();
    TestConfig::set_atomdb_cache(false);

    //                          OPTIONS="context atomdb_type use_proxy num_threads num_links arity
    //                          chunck_size"
    //
    // make run-tests-db-loader OPTIONS="test_1m_ redismongodb false 8 1000000 3 5000"
    // make run-tests-db-loader OPTIONS="test_1m_ morkdb false 8 1000000 3 5000"
    // make run-tests-db-loader OPTIONS="test_1m_ redismongodb true 8 1000000 3 5000"

    // From file
    // make run-tests-db-loader OPTIONS="test_1m_ redismongodb true 8 file /path/to/file.metta 5000"

    string context = "";
    if (argc > 1) context = string(argv[1]);
    string atomdb_type = "redismongodb";
    if (argc > 2) atomdb_type = string(argv[2]);

    if (atomdb_type == "redismongodb") {
        AtomDBSingleton::provide(shared_ptr<AtomDB>(new RedisMongoDB("")));
    } else if (atomdb_type == "morkdb") {
        AtomDBSingleton::provide(shared_ptr<AtomDB>(new MorkDB(context)));
    } else {
        Utils::error("Invalid atomdb type: " + atomdb_type);
        return 1;
    }

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    if (argc > 4) {
        auto atomdb = AtomDBSingleton::get_instance();
        auto db = dynamic_pointer_cast<RedisMongoDB>(atomdb);

        LOG_INFO("Dropping all databases...");
        db->drop_all();
        LOG_INFO("Databases dropped");

        int num_threads = Utils::string_to_int(argv[3]);

        bool use_proxy =
            string(argv[4]) == "proxy" || string(argv[4]) == "1" || string(argv[4]) == "true";
        if (use_proxy) {
            string atomdb_broker_client_id = "0.0.0.0:62002";
            string atomdb_broker_server_id = "0.0.0.0:40007";
            ServiceBusSingleton::init(atomdb_broker_client_id, atomdb_broker_server_id, 62003, 62099);
            Utils::sleep(1000);
        }

        string arg4 = string(argv[5]);
        vector<string> file_keywords = {"file", "--file", "FILE"};
        bool is_file_mode =
            find(file_keywords.begin(), file_keywords.end(), arg4) != file_keywords.end();

        if (is_file_mode) {
            STOP_WATCH_START(tests_db_loader_from_file);

            if (argc < 7) {
                Utils::error(
                    "File mode requires file path as argument. Usage: context atomdb_type num_threads "
                    "file "
                    "<file_path> [chunk_size]");
                return 1;
            }

            string file_path = argv[6];
            int chunk_size = 1000;  // Batch size for adding atoms to DB
            if (argc > 7) {
                chunk_size = Utils::string_to_int(argv[7]);
            }

            // First, read all lines from the file to determine line ranges for each thread
            ifstream file(file_path);
            if (!file.is_open()) {
                Utils::error("Failed to open file: " + file_path);
                return 1;
            }

            // Read all lines first to determine line ranges for each thread
            vector<string> lines;
            string line;
            while (getline(file, line)) {
                if (!line.empty()) {
                    lines.push_back(line);
                }
            }
            file.close();

            if (lines.empty()) {
                LOG_INFO("File is empty or contains no valid lines");
                return 0;
            }

            // Calculate lines per thread
            size_t lines_per_thread = lines.size() / num_threads;
            size_t remainder = lines.size() % num_threads;

            LOG_INFO("Processing " + to_string(lines.size()) + " lines with " + to_string(num_threads) +
                     " threads (chunk size: " + to_string(chunk_size) + " | lines per thread: " +
                     to_string(lines_per_thread) + " | remainder: " + to_string(remainder) + ")");

            vector<thread> threads;
            atomic<size_t> total_atoms_processed(0);

            shared_ptr<AtomDBProxy> proxy = nullptr;
            if (use_proxy) {
                proxy = make_shared<AtomDBProxy>();
                ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
            }

            for (int i = 0; i < num_threads; i++) {
                size_t start_line = i * lines_per_thread;
                size_t end_line = start_line + lines_per_thread;

                // Remainder must be added to the last thread
                if (i == num_threads - 1) {
                    end_line += remainder;
                }

                threads.emplace_back([&, start_line, end_line, i]() -> void {
                    vector<Atom*> batch_atoms;
                    vector<shared_ptr<MettaParserActions>>
                        parser_actions_list;  // Keep parser_actions alive
                    size_t thread_atoms_count = 0;
                    set<string> handles;      // Avoid deduplication

                    for (size_t line_idx = start_line; line_idx < end_line; line_idx++) {
                        const string& metta_expr = lines[line_idx];

                        try {
                            // Create parser actions for this line
                            shared_ptr<MettaParserActions> parser_actions =
                                make_shared<MettaParserActions>();

                            // Parse the S-Expression
                            MettaParser parser(metta_expr, parser_actions);
                            bool parse_error = parser.parse(false);  // Don't throw on error

                            if (parse_error) {
                                LOG_ERROR("Thread " + to_string(i) + " failed to parse line " +
                                          to_string(line_idx) + ": " + metta_expr);
                                continue;
                            }

                            // Keep parser_actions alive by storing it
                            parser_actions_list.push_back(parser_actions);

                            // Convert shared_ptr<Atom> to Atom* for add_atoms
                            for (const auto& pair : parser_actions->handle_to_atom) {
                                // Deduplicate atoms by handle within this thread
                                if (handles.find(pair.second->handle()) != handles.end()) {
                                    continue;
                                }
                                handles.insert(pair.second->handle());
                                batch_atoms.push_back(pair.second.get());
                                thread_atoms_count++;

                                // Add atoms in batches
                                if (batch_atoms.size() >= static_cast<size_t>(chunk_size)) {
                                    if (use_proxy) {
                                        proxy->add_atoms(batch_atoms);
                                    } else {
                                        add_atoms_via_atomdb(batch_atoms, false, true);
                                    }
                                    batch_atoms.clear();
                                    // Clear old parser_actions that are no longer needed
                                    // Keep only the last few to avoid memory buildup
                                    if (parser_actions_list.size() > 10) {
                                        parser_actions_list.erase(parser_actions_list.begin(),
                                                                  parser_actions_list.end() - 5);
                                    }
                                }
                            }
                        } catch (const exception& e) {
                            LOG_ERROR("Thread " + to_string(i) + " exception on line " +
                                      to_string(line_idx) + ": " + string(e.what()));
                        }
                    }

                    // Add remaining atoms
                    if (!batch_atoms.empty()) {
                        if (use_proxy) {
                            proxy->add_atoms(batch_atoms);
                        } else {
                            add_atoms_via_atomdb(batch_atoms, false, true);
                        }
                    }

                    total_atoms_processed += thread_atoms_count;
                    LOG_DEBUG("Thread " + to_string(i) + " processed " + to_string(thread_atoms_count) +
                              " atoms " + "(lines " + to_string(start_line) + "-" +
                              to_string(end_line - 1) + ")");
                });
            }

            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }

            LOG_INFO("Completed processing. Total atoms processed: " +
                     to_string(total_atoms_processed.load()));

            STOP_WATCH_FINISH(tests_db_loader_from_file, "TestsDBLoaderFromFile");

        } else {
            int num_links = Utils::string_to_int(argv[5]);
            int arity = Utils::string_to_int(argv[6]);
            int chunck_size = Utils::string_to_int(argv[7]);

            int links_per_thread = num_links / num_threads;
            int remainder = num_links % num_threads;

            vector<thread> threads;

            STOP_WATCH_START(tests_db_loader);
            shared_ptr<AtomDBProxy> proxy = nullptr;
            if (use_proxy) {
                proxy = make_shared<AtomDBProxy>();
                ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
            }

            for (int i = 0; i < num_threads; i++) {
                threads.emplace_back([&, thread_id = i, links_per_thread, remainder]() -> void {
                    vector<Link*> links;
                    vector<Node*> nodes;

                    int links_to_add = links_per_thread;
                    if (thread_id == num_threads - 1) {
                        links_to_add += remainder;
                    }

                    LOG_DEBUG("[" + to_string(thread_id) + "] Adding " + to_string(links_to_add * 2) +
                              " links and " + to_string(links_to_add * arity) + " nodes");

                    for (int j = 1; j <= links_to_add; j++) {
                        string metta_expression = "(";
                        vector<string> node_names;
                        vector<string> targets;
                        for (int k = 1; k <= arity; k++) {
                            string name = "add-links-T" + to_string(thread_id) + "-" + to_string(j) +
                                          "-" + to_string(k);
                            auto node = new Node("Symbol", name);
                            targets.push_back(node->handle());
                            nodes.push_back(node);
                            node_names.push_back(name);
                            metta_expression += name;
                            if (k != arity) {
                                metta_expression += " ";
                            }
                        }
                        metta_expression += ")";

                        auto link = new Link("Expression", targets, true, {}, metta_expression);
                        links.push_back(link);

                        vector<string> nested_targets = {targets[0], targets[1], link->handle()};
                        string nested_metta_expression =
                            "(" + node_names[0] + " " + node_names[1] + " " + metta_expression + ")";
                        auto link_with_nested =
                            new Link("Expression", nested_targets, true, {}, nested_metta_expression);
                        links.push_back(link_with_nested);

                        if (j % chunck_size == 0) {
                            vector<Atom*> atoms;
                            for (auto* node : nodes) atoms.push_back(node);
                            for (auto* link : links) atoms.push_back(link);
                            if (use_proxy) {
                                proxy->add_atoms(atoms);
                            } else {
                                add_atoms_via_atomdb(atoms, false, true);
                            }
                            // Delete nodes and links after adding to database
                            for (auto* node : nodes) {
                                delete node;
                            }
                            for (auto* link : links) {
                                delete link;
                            }
                            nodes.clear();
                            links.clear();
                        }
                    }

                    if (!nodes.empty() || !links.empty()) {
                        LOG_DEBUG("[" + to_string(thread_id) + "] Final - Adding " +
                                  to_string(nodes.size()) + " nodes and " + to_string(links.size()) +
                                  " links");
                        vector<Atom*> atoms;
                        for (auto* node : nodes) atoms.push_back(node);
                        for (auto* link : links) atoms.push_back(link);
                        if (use_proxy) {
                            proxy->add_atoms(atoms);
                        } else {
                            add_atoms_via_atomdb(atoms, false, true);
                        }
                        // Delete nodes after adding to database
                        for (auto* atom : atoms) {
                            delete atom;
                        }
                    }

                    // clang-format off
                    LinkSchema link_schema({
                        "LINK_TEMPLATE", "Expression", "3", 
                        "NODE", "Symbol", "add-links-T" + to_string(thread_id) + "-1-1", 
                        "NODE", "Symbol", "add-links-T" + to_string(thread_id) + "-1-2",
                        "VARIABLE", "V"
                    });
                    // clang-format on

                    auto result = AtomDBSingleton::get_instance()->query_for_pattern(link_schema);
                    auto timeout_counter = 0;
                    while (result->size() != 2) {
                        Utils::sleep(100);
                        result = AtomDBSingleton::get_instance()->query_for_pattern(link_schema);
                        timeout_counter++;
                        if (timeout_counter >= 10 * 60) {  // 60 seconds
                            Utils::error("[" + to_string(thread_id) +
                                         "] Timeout waiting for pattern query results");
                            return;
                        }
                    }
                    if (result->size() != 2) {
                        Utils::error("[" + to_string(thread_id) + "] Expected 2 results, got " +
                                     to_string(result->size()));
                        return;
                    }

                    auto iterator = result->get_iterator();
                    char* handle;
                    while ((handle = iterator->next()) != nullptr) {
                        LOG_DEBUG("[" + to_string(thread_id) + "] Match: " + string(handle));
                    }
                });
            }

            for (auto& thread : threads) {
                thread.join();
            }

            STOP_WATCH_FINISH(tests_db_loader, "TestsDBLoader");
        }
    } else {
        LOG_INFO("Starting tests_db_loader (context: '" + context + "')");
        load_animals_data();
        LOG_INFO("tests_db_loader done (context: '" + context + "')");
    }

    return 0;
}
