#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "MockAnimalsData.h"
#include "TestConfig.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;

void ctrl_c_handler(int) {
    cout << "Stopping tests_db_loader..." << endl;
    cout << "Done." << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    TestConfig::load_environment();
    TestConfig::disable_atomdb_cache();

    string context = "";
    if (argc > 1) context = string(argv[1]);

    auto atomdb = new RedisMongoDB(context);
    AtomDBSingleton::provide(shared_ptr<AtomDB>(atomdb));

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    LOG_INFO("Starting tests_db_loader (context: '" + context + "')");

    load_animals_data();

    LOG_INFO("tests_db_loader done (context: '" + context + "')");

    return 0;
}
