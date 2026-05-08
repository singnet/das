#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "AtomDB.h"
#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "JsonConfig.h"
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
using namespace commons;

mutex global_mutex;
map<string, Metrics> global_metrics;

namespace {

JsonConfig benchmark_atomdb_config(string atomdb_type) {
    auto json = nlohmann::json();
    json["type"] = atomdb_type;
    json["redis"] = {"endpoint" : "localhost:40020", "cluster" : false};
    json["mongodb"] = {"endpoint" : "localhost:40021", "username" : "admin", "password" : "admin"};
    json["morkdb"] = {"endpoint" : "localhost:40022"};
    return JsonConfig(json);
}

}  // namespace

void setup(string atomdb_type, string client_id, string server_id) {
    AtomDBSingleton::init(benchmark_atomdb_config(atomdb_type));
    ServiceBusSingleton::init(client_id, server_id, 4000, 4100);
}

int main(int argc, char** argv) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0]
             << " <report_base_directory> <atomdb_type> <action> <num_iterations> "
             << "[server_host:port] [client_host:port]" << endl;
        exit(1);
    }

    string report_base_directory = argv[1];
    string atomdb_type = argv[2];
    string action = argv[3];
    int iterations = stoi(argv[4]);
    string base_log_file = argv[5];
    string server_id = "0.0.0.0:35700";
    string client_id = "0.0.0.0:9000";

    if (argc > 6) {
        server_id = argv[6];
    }
    if (argc > 7) {
        client_id = argv[7];
    }

    setup(atomdb_type, client_id, server_id);

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
        RAISE_ERROR(
            "Invalid action. Choose either SimpleQuery, PositiveImportance, ComplexQuery, "
            "UpdateAttentionBroker, or LinkTemplateCache.");
    }

    string filename = report_base_directory + "/" + "query_agent_" + atomdb_type + "_" + action + "_" +
                      "method" + "_1" + ".txt";

    create_report(filename, global_metrics);

    return 0;
}
