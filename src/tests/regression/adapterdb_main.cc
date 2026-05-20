#include <signal.h>

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>

#include "AdapterDB.h"
#include "AtomDBSingleton.h"
#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "MorkDB.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace atomdb;

void ctrl_c_handler(int) {
    std::cout << "Stopping database adapter..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

void usage(const char* a) {
    cerr << "Usage: " << a
         << " <config_file>\n\n"
            "Examples:\n"
            "  ./bin config.json\n";
    exit(1);
}

commons::JsonConfig morkdb_json_config() {
    auto json = nlohmann::json();
    json["type"] = "morkdb";
    json["mongodb"] = {{"endpoint", "localhost:40021"}, {"username", "admin"}, {"password", "admin"}};
    json["morkdb"] = {{"endpoint", "localhost:40022"}};
    return commons::JsonConfig(json);
}

void compare_counts(const string& name, size_t count1, size_t count2) {
    if (count1 != count2) {
        LOG_ERROR(name << " count mismatch: " << count1 << " vs " << count2);
    } else {
        LOG_INFO(name << " count matches: " << count1);
    }
}

tuple<size_t, size_t, size_t> read_expected_counts(const string& file_path) {
    ifstream infile(file_path);
    if (!infile.is_open()) {
        LOG_ERROR("Failed to open expected counts file: " << file_path);
        return {0, 0, 0};
    }

    size_t node_count = 0;
    size_t link_count = 0;
    size_t atom_count = 0;

    infile >> node_count >> link_count >> atom_count;
    infile.close();

    return {node_count, link_count, atom_count};
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        usage(argv[0]);
    }

    LOG_INFO("Starting database adapter...");

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    JsonConfig json_config = JsonConfigParser::load(argv[1]);

    auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());

    AtomDBSingleton::init(atomdb_config);

    auto morkdb = make_shared<MorkDB>("", morkdb_json_config());

    size_t nodes = morkdb->node_count();
    size_t links = morkdb->link_count();
    size_t atoms = morkdb->atom_count();

    LOG_INFO("Node count: " << nodes);
    LOG_INFO("Link count: " << links);
    LOG_INFO("Atom count: " << atoms);

    auto [expected_nodes, expected_links, expected_atoms] =
        read_expected_counts("/opt/das/src/tests/assets/adapterdb_expected_counts.txt");

    compare_counts("Node", nodes, expected_nodes);
    compare_counts("Link", links, expected_links);
    compare_counts("Atom", atoms, expected_atoms);

    LOG_INFO("Database adapter stopped.");

    return 0;
}