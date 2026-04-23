#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "DatabaseAdapter.h"
#include "DatabaseTypes.h"
#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace atomdb;
using namespace db_adapter;

void ctrl_c_handler(int) {
    std::cout << "Stopping database adapter..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

void usage(const char* a) {
    cerr << "Usage: " << a
         << " <config_file> [save_metta] \n\n"
            "Examples:\n"
            "  ./bin config.json\n"
            "  ./bin config.json true\n";
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        usage(argv[0]);
    }

    LOG_INFO("Starting database adapter...");

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    string config = argv[1];
    bool save_metta = (argc >= 3) ? (string(argv[2]) == "true") : false;

    JsonConfig json_config = JsonConfigParser::load(config);

    auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
    AtomDBSingleton::init(atomdb_config);

    run_database_adapter(json_config, save_metta);

    LOG_INFO("Database adapter stopped.");

    return 0;
}