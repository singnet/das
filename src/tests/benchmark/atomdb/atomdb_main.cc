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
#include "AtomDBCacheSingleton.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "Utils.h"
#include "atomdb_operations.h"
#include "atomdb_runner.h"
#include "atomdb_utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;

using Clock = chrono::steady_clock;
const size_t BATCH_SIZE = 10;  // Number of atoms in a single batch

mutex global_mutex;
map<string, Metrics> global_metrics;

void setup(bool cache_enable) {
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
    AtomDBCacheSingleton::init();
    RedisMongoDB::initialize_statics();
}

shared_ptr<AtomDB> factory_create_atomdb(string type) {
    if (type == "redis_mongo") {
        return make_shared<RedisMongoDB>();
    } else if (type == "mork") {
        return make_shared<MorkDB>();
    } else {
        Utils::error("Unknown AtomDB type: " + type);
    }
}

template <typename BenchmarkClass>
void dispatch_handler(BenchmarkClass& benchmark,
                      const map<string, function<void()>>& handlers,
                      const string& method) {
    auto it = handlers.find(method);
    if (it != handlers.end()) {
        it->second();
    } else {
        Utils::error("Invalid method: " + method);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <atomdb_type> <action> <cache> <num_concurrency> <num_iterations> <timestamp>" << endl;
        exit(1);
    }

    string atomdb_type = argv[1];
    string action = argv[2];
    string method = argv[3];
    bool cache_enable = ((string(argv[4]) == "enabled") ? true : false);
    int concurrency = stoi(argv[5]);
    int iterations = stoi(argv[6]);
    string timestamp = argv[7];

    setup(cache_enable);
    auto atomdb = factory_create_atomdb(atomdb_type);

    auto worker = [&](int tid, shared_ptr<AtomDB> atomdb) {
        if (action == "AddAtom") {
            AddAtom benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"add_node", [&]() { benchmark.add_node(); }},
                {"add_link", [&]() { benchmark.add_link(); }},
                {"add_atom_node", [&]() { benchmark.add_atom_node(); }},
                {"add_atom_link", [&]() { benchmark.add_atom_link(); }},
            };
            dispatch_handler(benchmark, benchmark_handlers, method);
        } else if (action == "AddAtoms") {
            AddAtoms benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"add_nodes", [&]() { benchmark.add_nodes(); }},
                {"add_links", [&]() { benchmark.add_links(); }},
                {"add_atoms_node", [&]() { benchmark.add_atoms_node(); }},
                {"add_atoms_link", [&]() { benchmark.add_atoms_link(); }},
            };
            dispatch_handler(benchmark, benchmark_handlers, method);
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
            dispatch_handler(benchmark, benchmark_handlers, method);
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
            dispatch_handler(benchmark, benchmark_handlers, method);
        } else if (action == "DeleteAtom") {
            DeleteAtom benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"delete_node", [&]() { benchmark.delete_node(atomdb_type); }},
                {"delete_link", [&]() { benchmark.delete_link(atomdb_type); }},
                {"delete_atom_node", [&]() { benchmark.delete_atom_node(atomdb_type); }},
                {"delete_atom_link", [&]() { benchmark.delete_atom_link(atomdb_type); }},
            };
            dispatch_handler(benchmark, benchmark_handlers, method);
        } else if (action == "DeleteAtoms") {
            DeleteAtoms benchmark(tid, atomdb, iterations);
            map<string, function<void()>> benchmark_handlers{
                {"delete_nodes", [&]() { benchmark.delete_nodes(atomdb_type); }},
                {"delete_links", [&]() { benchmark.delete_links(atomdb_type); }},
                {"delete_atoms_node", [&]() { benchmark.delete_atoms_node(atomdb_type); }},
                {"delete_atoms_link", [&]() { benchmark.delete_atoms_link(atomdb_type); }},
            };
            dispatch_handler(benchmark, benchmark_handlers, method);
        } else {
            Utils::error(
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

    create_report(get_type_name(*atomdb), action, method, global_metrics, base_directory, batch_size);

    return 0;
}