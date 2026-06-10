#include <signal.h>

#include <atomic>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "AdapterDB.h"
#include "AtomDBSingleton.h"
#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "MettaParser.h"
#include "MettaParserActions.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "RemoteAtomDB.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;
using namespace metta;
using namespace atoms;
using namespace commons;

static string flag_from_argv_or(int argc,
                                char* argv[],
                                const string& flag,
                                const string& default_value) {
    for (int i = 1; i < argc; i++) {
        string arg(argv[i]);
        if (arg.rfind(flag, 0) == 0) {
            return arg.substr(flag.length());
        }
    }
    return default_value;
}

static bool has_flag(int argc, char* argv[], const string& flag) {
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]).rfind(flag, 0) == 0) {
            return true;
        }
    }
    return false;
}

void ctrl_c_handler(int) {
    cout << "Stopping db_loader..." << endl;
    cout << "Done." << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    // make run-db-loader OPTIONS="--config=config/das.json"
    //
    // make run-db-loader OPTIONS="--config=config/das.json --context=test_1m_ --threads=8
    // --links=1000000 --arity=3 --chunk=5000"
    //
    // make run-db-loader OPTIONS="--config=config/das.json --context=test_1m_ --file=/path/to/file.metta
    // --threads=8 --chunk=5000"

    string config_path = flag_from_argv_or(argc, argv, "--config=", "");
    if (config_path.empty()) {
        RAISE_ERROR("No --config=path provided");
        return 1;
    }

    string context = flag_from_argv_or(argc, argv, "--context=", "");
    string file_path = flag_from_argv_or(argc, argv, "--file=", "");

    int num_threads = Utils::string_to_int(flag_from_argv_or(argc, argv, "--threads=", "8"));
    int num_links = Utils::string_to_int(flag_from_argv_or(argc, argv, "--links=", "1000000"));
    int arity = Utils::string_to_int(flag_from_argv_or(argc, argv, "--arity=", "3"));
    int chunk_size = Utils::string_to_int(flag_from_argv_or(argc, argv, "--chunk=", "5000"));

    JsonConfig json_config = JsonConfigParser::load(config_path);
    auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());

    auto atomdb_type = atomdb_config.at_path("type").get_or<string>("");
    if (atomdb_type == "redismongodb") {
        AtomDBSingleton::provide(make_shared<RedisMongoDB>(context, false, atomdb_config));
    } else if (atomdb_type == "morkdb") {
        AtomDBSingleton::provide(make_shared<MorkDB>(context, atomdb_config));
    } else if (atomdb_type == "remotedb") {
        AtomDBSingleton::provide(make_shared<RemoteAtomDB>(atomdb_config));
    } else if (atomdb_type == "adapterdb") {
        AtomDBSingleton::provide(make_shared<AdapterDB>(atomdb_config));
    } else {
        RAISE_ERROR("Invalid AtomDB type: " + atomdb_type);
    }

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    if (!file_path.empty()) {
        STOP_WATCH_START(db_loader_from_file);

        ifstream file(file_path);
        if (!file.is_open()) {
            RAISE_ERROR("Failed to open file: " + file_path);
            return 1;
        }

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

        size_t lines_per_thread = lines.size() / num_threads;
        size_t remainder = lines.size() % num_threads;

        LOG_INFO("Processing " + to_string(lines.size()) + " lines with " + to_string(num_threads) +
                 " threads (chunk size: " + to_string(chunk_size) + " | lines per thread: " +
                 to_string(lines_per_thread) + " | remainder: " + to_string(remainder) + ")");

        vector<thread> threads;
        atomic<size_t> total_atoms_processed(0);

        for (int i = 0; i < num_threads; i++) {
            size_t start_line =
                (static_cast<size_t>(i) * lines.size()) / static_cast<size_t>(num_threads);
            size_t end_line =
                (static_cast<size_t>(i + 1) * lines.size()) / static_cast<size_t>(num_threads);

            threads.emplace_back([&, start_line, end_line, i]() -> void {
                auto thread_atomdb = AtomDBSingleton::get_instance();
                vector<Atom*> batch_atoms;
                vector<shared_ptr<MettaParserActions>> parser_actions_list;
                size_t thread_atoms_count = 0;
                set<string> handles;

                for (size_t line_idx = start_line; line_idx < end_line; line_idx++) {
                    const string& metta_expr = lines[line_idx];

                    try {
                        shared_ptr<MettaParserActions> parser_actions =
                            make_shared<MettaParserActions>();
                        MettaParser parser(metta_expr, parser_actions);
                        bool parse_error = parser.parse(false);

                        if (parse_error) {
                            LOG_ERROR("Thread " + to_string(i) + " failed to parse line " +
                                      to_string(line_idx) + ": " + metta_expr);
                            continue;
                        }

                        parser_actions_list.push_back(parser_actions);

                        for (const auto& pair : parser_actions->handle_to_atom) {
                            if (handles.find(pair.second->handle()) != handles.end()) {
                                continue;
                            }
                            handles.insert(pair.second->handle());

                            auto atom = pair.second.get();
                            if (atom->arity() > 0) {
                                atom->custom_attributes["metta_expression"] = metta_expr;
                            }
                            batch_atoms.push_back(atom);
                            thread_atoms_count++;

                            if (batch_atoms.size() >= static_cast<size_t>(chunk_size)) {
                                thread_atomdb->add_atoms(batch_atoms, false, true);
                                batch_atoms.clear();
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

                if (!batch_atoms.empty()) {
                    thread_atomdb->add_atoms(batch_atoms, false, true);
                }

                total_atoms_processed += thread_atoms_count;
                LOG_INFO("Thread " + to_string(i) + " processed " + to_string(thread_atoms_count) +
                         " atoms (lines " + to_string(start_line) + "-" + to_string(end_line - 1) + ")");
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        LOG_INFO("Completed processing. Total atoms processed: " +
                 to_string(total_atoms_processed.load()));
        STOP_WATCH_FINISH(db_loader_from_file, "DBLoaderFromFile");

    } else {
        auto atomdb = AtomDBSingleton::get_instance();
        auto db = dynamic_pointer_cast<RedisMongoDB>(atomdb);
        if (db != nullptr) {
            try {
                LOG_INFO("Dropping all databases...");
                db->drop_all();
                LOG_INFO("Databases dropped");
            } catch (const exception& e) {
                LOG_ERROR("Failed to drop databases: " + string(e.what()));
            }
        }

        int links_per_thread = num_links / num_threads;
        int remainder = num_links % num_threads;

        vector<thread> threads;

        STOP_WATCH_START(db_loader);

        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back([&, thread_id = i, links_per_thread, remainder]() -> void {
                auto thread_db = AtomDBSingleton::get_instance();

                vector<Link*> links;
                vector<Node*> nodes;

                int links_to_add = links_per_thread;
                if (thread_id == num_threads - 1) {
                    links_to_add += remainder;
                }

                LOG_INFO("[" + to_string(thread_id) + "] Adding " + to_string(links_to_add * 2) +
                         " links and " + to_string(links_to_add * arity) + " nodes");

                for (int j = 1; j <= links_to_add; j++) {
                    string metta_expression = "(";
                    vector<string> node_names;
                    vector<string> targets;
                    for (int k = 1; k <= arity; k++) {
                        string name = "add-links-T" + to_string(thread_id) + "-" + to_string(j) + "-" +
                                      to_string(k);
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

                    auto link = new Link(
                        "Expression", targets, true, Properties{{"metta_expression", metta_expression}});
                    links.push_back(link);

                    vector<string> nested_targets = {targets[0], targets[1], link->handle()};
                    string nested_metta_expression =
                        "(" + node_names[0] + " " + node_names[1] + " " + metta_expression + ")";
                    auto link_with_nested =
                        new Link("Expression",
                                 nested_targets,
                                 true,
                                 Properties{{"metta_expression", nested_metta_expression}});
                    links.push_back(link_with_nested);

                    if (j % chunk_size == 0) {
                        thread_db->add_nodes(nodes, false, true);
                        thread_db->add_links(links, false, true);
                        nodes.clear();
                        links.clear();
                    }
                }

                if (!nodes.empty()) {
                    LOG_INFO("[" + to_string(thread_id) + "] Final - Adding " + to_string(nodes.size()) +
                             " nodes");
                    thread_db->add_nodes(nodes, false, true);
                }
                if (!links.empty()) {
                    LOG_INFO("[" + to_string(thread_id) + "] Final - Adding " + to_string(links.size()) +
                             " links");
                    thread_db->add_links(links, false, true);
                }

                // clang-format off
                LinkSchema link_schema({
                    "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "add-links-T" + to_string(thread_id) + "-1-1",
                    "NODE", "Symbol", "add-links-T" + to_string(thread_id) + "-1-2",
                    "VARIABLE", "V"
                });
                // clang-format on

                auto result = thread_db->query_for_pattern(link_schema);
                if (result->size() != 2) {
                    RAISE_ERROR("[" + to_string(thread_id) + "] Expected 2 results, got " +
                                to_string(result->size()));
                    return;
                }

                auto iterator = result->get_iterator();
                char* handle;
                while ((handle = iterator->next()) != nullptr) {
                    LOG_INFO("[" + to_string(thread_id) + "] Match: " + string(handle));
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        STOP_WATCH_FINISH(db_loader, "DBLoader");
    }

    return 0;
}
