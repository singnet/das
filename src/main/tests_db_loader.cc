#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "TestConfig.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"
#include "MockAnimalsData.h"

using namespace std;
using namespace atomdb;

void ctrl_c_handler(int) {
    cout << "Stopping tests_db_loader..." << endl;
    cout << "Done." << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    TestConfig::load_environment();
    TestConfig::set_atomdb_cache(false);

    //                          OPTIONS="context atomdb_type num_links arity chunck_size"
    //
    // make run-tests-db-loader OPTIONS="test_1m_ redismongodb 1000000 3 5000"
    // make run-tests-db-loader OPTIONS="test_1m_ morkdb 1000000 3 5000"

    string context = "";
    if (argc > 1) context = string(argv[1]);
    string atomdb_type = "redismongodb";
    if (argc > 2) atomdb_type = string(argv[2]);

    if (atomdb_type == "redismongodb") {
        AtomDBSingleton::provide(shared_ptr<AtomDB>(new RedisMongoDB(context)));
    } else if (atomdb_type == "morkdb") {
        AtomDBSingleton::provide(shared_ptr<AtomDB>(new MorkDB(context)));
    } else {
        Utils::error("Invalid atomdb type: " + atomdb_type);
        return 1;
    }

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    if (argc > 5) {
        auto atomdb = AtomDBSingleton::get_instance();
        auto db = dynamic_pointer_cast<RedisMongoDB>(atomdb);

        LOG_INFO("Dropping all databases...");
        db->drop_all();
        LOG_INFO("Databases dropped");

        int num_links = Utils::string_to_int(argv[3]);
        int arity = Utils::string_to_int(argv[4]);
        int chunck_size = Utils::string_to_int(argv[5]);

        vector<Link*> links;
        vector<Node*> nodes;

        STOP_WATCH_START(tests_db_loader);

        for (int i = 1; i <= num_links; i++) {
            string metta_expression = "(";
            vector<string> node_names;
            vector<string> targets;
            for (int j = 1; j <= arity; j++) {
                string name = "add-links-" + to_string(i) + "-" + to_string(j);
                auto node = new Node("Symbol", name);
                targets.push_back(node->handle());
                nodes.push_back(node);
                node_names.push_back(name);
                metta_expression += name;
                if (j != arity) {
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

            if (i % chunck_size == 0) {
                LOG_DEBUG("Adding " + to_string(nodes.size()) + " nodes");
                db->add_nodes(nodes, false, true);
                LOG_DEBUG("Adding " + to_string(links.size()) + " links");
                db->add_links(links, false, true);
                nodes.clear();
                links.clear();
            }
        }

        if (!nodes.empty()) {
            LOG_DEBUG("Final - Adding " + to_string(nodes.size()) + " nodes");
            db->add_nodes(nodes, false, true);
        }
        if (!links.empty()) {
            LOG_DEBUG("Final - Adding " + to_string(links.size()) + " links");
            db->add_links(links, false, true);
        }

        STOP_WATCH_FINISH(tests_db_loader, "TestsDBLoader");

        // clang-format off
        LinkSchema link_schema({
            "LINK_TEMPLATE", "Expression", "3", 
            "NODE", "Symbol", "add-links-1-1", 
            "NODE", "Symbol", "add-links-1-2",
            "VARIABLE", "V"
        });
        // clang-format on

        auto result = db->query_for_pattern(link_schema);
        if (result->size() != 2) {
            Utils::error("Expected 2 results, got " + to_string(result->size()));
            return 1;
        }

        auto iterator = result->get_iterator();
        char* handle;
        while ((handle = iterator->next()) != nullptr) {
            LOG_DEBUG("Match: " + string(handle));
        }
    } else {
        LOG_INFO("Starting tests_db_loader (context: '" + context + "')");
        load_animals_data();
        LOG_INFO("tests_db_loader done (context: '" + context + "')");
    }

    return 0;
}
