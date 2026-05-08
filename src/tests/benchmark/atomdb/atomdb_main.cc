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
#include "JsonConfig.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "Utils.h"
#include "atomdb_operations.h"
#include "atomdb_runner.h"
#include "benchmark_utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;
using namespace commons;

const size_t BATCH_SIZE = 10;  // Number of atoms in a single batch

namespace {

/** Benchmark compose ports (see scripts/run_benchmark.sh). */
JsonConfig benchmark_atomdb_config() {
    return JsonConfig(nlohmann::json::parse(R"({
        "redis": { "endpoint": "localhost:29000", "cluster": false },
        "mongodb": {
            "endpoint": "localhost:28000",
            "username": "dbadmin",
            "password": "dassecret"
        },
        "morkdb": { "endpoint": "localhost:8000" }
    })"));
}

}  // namespace

mutex global_mutex;
map<string, Metrics> global_metrics;

void setup() { RedisMongoDB::initialize_statics(); }

shared_ptr<AtomDB> factory_create_atomdb(string type, const JsonConfig& atomdb_config) {
    if (type == "redismongodb") {
        return make_shared<RedisMongoDB>("", false, atomdb_config);
    } else if (type == "morkdb") {
        return make_shared<MorkDB>("", atomdb_config);
    } else {
        RAISE_ERROR("Unknown AtomDB type: " + type);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <atomdb_type> <action> <num_concurrency> <num_iterations> <timestamp>" << endl;
        exit(1);
    }

    string atomdb_type = argv[1];
    string action = argv[2];
    string method = argv[3];
    int concurrency = stoi(argv[5]);
    int iterations = stoi(argv[6]);
    string timestamp = argv[7];

    setup();
    auto atomdb = factory_create_atomdb(atomdb_type, benchmark_atomdb_config());

    auto worker = [&](int tid, shared_ptr<AtomDB> atomdb) {
        if (action == "AddAtom") {
            AddAtom benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"add_node", [&]() { benchmark.add_node(); }},
                {"add_link", [&]() { benchmark.add_link(); }},
                {"add_atom_node", [&]() { benchmark.add_atom_node(); }},
                {"add_atom_link", [&]() { benchmark.add_atom_link(); }},
            };
            dispatch_handler(benchmark_handlers, method);
        } else if (action == "AddAtoms") {
            AddAtoms benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"add_nodes", [&]() { benchmark.add_nodes(); }},
                {"add_links", [&]() { benchmark.add_links(); }},
                {"add_atoms_node", [&]() { benchmark.add_atoms_node(); }},
                {"add_atoms_link", [&]() { benchmark.add_atoms_link(); }},
            };
            dispatch_handler(benchmark_handlers, method);
        } else if (action == "GetAtom") {
            GetAtom benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"get_node_document", [&]() { benchmark.get_node_document(); }},
                {"get_link_document", [&]() { benchmark.get_link_document(); }},
                {"get_atom_document_node", [&]() { benchmark.get_atom_document_node(); }},
                {"get_atom_document_link", [&]() { benchmark.get_atom_document_link(); }},
                {"get_atom_node", [&]() { benchmark.get_atom_node(); }},
                {"get_atom_link", [&]() { benchmark.get_atom_link(); }},
            };
            dispatch_handler(benchmark_handlers, method);
        } else if (action == "GetAtoms") {
            GetAtoms benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"get_node_documents", [&]() { benchmark.get_node_documents(); }},
                {"get_link_documents", [&]() { benchmark.get_link_documents(); }},
                {"get_atom_documents_node", [&]() { benchmark.get_atom_documents_node(); }},
                {"get_atom_documents_link", [&]() { benchmark.get_atom_documents_link(); }},
                {"query_for_pattern", [&]() { benchmark.query_for_pattern(); }},
                {"query_for_targets", [&]() { benchmark.query_for_targets(); }},
            };
            dispatch_handler(benchmark_handlers, method);
        } else if (action == "DeleteAtom") {
            DeleteAtom benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"delete_node", [&]() { benchmark.delete_node(atomdb_type); }},
                {"delete_link", [&]() { benchmark.delete_link(atomdb_type); }},
                {"delete_atom_node", [&]() { benchmark.delete_atom_node(atomdb_type); }},
                {"delete_atom_link", [&]() { benchmark.delete_atom_link(atomdb_type); }},
            };
            dispatch_handler(benchmark_handlers, method);
        } else if (action == "DeleteAtoms") {
            DeleteAtoms benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"delete_nodes", [&]() { benchmark.delete_nodes(atomdb_type); }},
                {"delete_links", [&]() { benchmark.delete_links(atomdb_type); }},
                {"delete_atoms_node", [&]() { benchmark.delete_atoms_node(atomdb_type); }},
                {"delete_atoms_link", [&]() { benchmark.delete_atoms_link(atomdb_type); }},
            };
            dispatch_handler(benchmark_handlers, method);
        } else {
            RAISE_ERROR(
                "Invalid action. Choose either 'AddAtom' or 'AddAtoms' or 'GetAtom' or 'GetAtoms' "
                "or 'DeleteAtom' or 'DeleteAtoms'");
        }
    };

    vector<thread> threads;
    for (int t = 0; t < concurrency; t++) {
        threads.emplace_back(worker, t, atomdb);
    }
    for (auto& th : threads) {
        th.join();
    }

    string base_directory = "/tmp/atomdb_benchmark/" + timestamp;
    int batch_size = (action == "AddAtoms" || action == "DeleteAtoms") ? BATCH_SIZE : 1;

    string filename = base_directory + "/" + "atomdb_" + atomdb_type + "_" + action + "_" + method +
                      "_" + to_string(batch_size) + ".txt";

    create_report(filename, global_metrics);

    return 0;
}