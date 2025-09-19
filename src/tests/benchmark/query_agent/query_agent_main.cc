#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "AtomDB.h"
#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "MorkDB.h"
#include "PatternMatchingQueryProxy.h"
#include "RedisMongoDB.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "benchmark_utils.h"
#include "query_agent_operations.h"
#include "query_agent_runner.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace std;
using namespace query_engine;
using namespace service_bus;
using namespace atom_space;
using namespace atomdb;

mutex global_mutex;
map<string, Metrics> global_metrics;

void setup(bool cache_enable, string atomdb_type, string client_id, string server_id) {
    setenv("DAS_REDIS_HOSTNAME", "localhost", 1);
    setenv("DAS_REDIS_PORT", "29000", 1);
    setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
    setenv("DAS_MONGODB_HOSTNAME", "localhost", 1);
    setenv("DAS_MONGODB_PORT", "28000", 1);
    setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
    setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);
    setenv("DAS_MORK_HOSTNAME", "localhost", 1);
    setenv("DAS_MORK_PORT", "8000", 1);
    setenv("DAS_DISABLE_ATOMDB_CACHE", cache_enable ? "false" : "true", 1);
    if (atomdb_type == "redismongodb") {
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::REDIS_MONGODB);
    } else if (atomdb_type == "morkdb") {
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
    }
    ServiceBusSingleton::init(client_id, server_id, 4000, 4100);
}

int main(int argc, char** argv) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0]
             << " <report_base_directory> <atomdb_type> <action> <cache_enabled> <num_iterations> "
             << "[server_host:port] [client_host:port]"
             << endl;
        exit(1);
    }

    string report_base_directory = argv[1];
    string atomdb_type = argv[2];
    string action = argv[3];
    bool cache_enable = (string(argv[4]) == "true" || string(argv[4]) == "1");
    int iterations = stoi(argv[5]);
    string base_log_file = argv[6];
    string server_id = "0.0.0.0:35700";
    string client_id = "0.0.0.0:9000";

    if (argc > 7) {
        server_id = argv[7];
    }
    if (argc > 8) {
        client_id = argv[8];
    }

    setup(cache_enable, atomdb_type, client_id, server_id);

    auto atom_space = make_shared<AtomSpace>();
    PatternMatchingQuery benchmark(1, atom_space, iterations);

    if (action == "SimpleQuery") {
        benchmark.simple_query(base_log_file);
    } else if (action == "PositiveImportance") {
        benchmark.positive_importance();
    } else if (action == "ComplexQuery") {
        benchmark.complex_query(base_log_file);
    } else if (action == "UpdateAttentionBroker") {
        benchmark.update_attention_broker();
    } else if (action == "LinkTemplateCache") {
        benchmark.link_template_cache();
    } else {
        Utils::error(
            "Invalid action. Choose either SimpleQuery, PositiveImportance, ComplexQuery, "
            "UpdateAttentionBroker, or LinkTemplateCache.");
    }

    string filename = report_base_directory + "/" + "query_agent_" + atomdb_type + "_" + action + "_" +
                      "method" + "_1" + ".txt";

    create_report(filename, global_metrics);

    return 0;
}
