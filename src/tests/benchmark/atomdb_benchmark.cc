#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "Logger.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "Utils.h"
#include "atomdb_benchmark_utils.h"

#define LOG_LEVEL DEBUG_LEVEL

using namespace std;
using namespace atomdb;

using Clock = chrono::steady_clock;

mutex global_mutex;
map<string, vector<double>> global_latencies;

void add_atom(int tid, shared_ptr<AtomDB> db, int iterations) {
    vector<double> local_add_node_latencies;
    vector<double> local_add_link_latencies;

    local_add_node_latencies.reserve(iterations);
    local_add_link_latencies.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        auto node_a = new Node(
            "Symbol", string("\"add_node_a") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");

        auto t0 = Clock::now();
        db->add_node(node_a);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_add_node_latencies.push_back(ms);
    }

    for (int i = 0; i < iterations; i++) {
        auto node_equivalence =
            new Node("Symbol", string("EQUIVALENCE") + "_t" + to_string(tid) + "_i" + to_string(i));
        auto node_equivalence_handle = db->add_node(node_equivalence);
        auto node_a = new Node(
            "Symbol", string("\"add_node_b") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_a_handle = db->add_node(node_a);
        auto node_b = new Node(
            "Symbol", string("\"add_node_c") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_b_handle = db->add_node(node_b);

        auto link = new Link("Expression", {node_equivalence_handle, node_a_handle, node_b_handle});

        auto t0 = Clock::now();
        db->add_link(link);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_add_link_latencies.push_back(ms);
    }

    lock_guard<mutex> lock(global_mutex);
    global_latencies["add_node"].insert(global_latencies["add_node"].end(),
                                        local_add_node_latencies.begin(),
                                        local_add_node_latencies.end());
    global_latencies["add_link"].insert(global_latencies["add_link"].end(),
                                        local_add_link_latencies.begin(),
                                        local_add_link_latencies.end());
}
void add_atoms(int tid, shared_ptr<AtomDB> db, int iterations) {    /* WIP */
}
void get_atom(int tid, shared_ptr<AtomDB> db, int iterations) {     /* WIP */
}
void get_atoms(int tid, shared_ptr<AtomDB> db, int iterations) {    /* WIP */
}
void delete_atom(int tid, shared_ptr<AtomDB> db, int iterations) {  /* WIP */
}
void delete_atoms(int tid, shared_ptr<AtomDB> db, int iterations) { /* WIP */
}

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
    if (cache_enable) {
        setenv("DAS_DISABLE_ATOMDB_CACHE", "false", 1);
    } else {
        setenv("DAS_DISABLE_ATOMDB_CACHE", "true", 1);
    }
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

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <atomdb_type> <action> <cache> <num_concurrency> <num_iterations>" << endl;
        exit(1);
    }

    string atomdb_type = argv[1];
    string action = argv[2];
    bool cache_enable = ((string(argv[3]) == "enabled") ? true : false);
    int concurrency = stoi(argv[4]);
    int iterations = stoi(argv[5]);

    setup(cache_enable);
    auto atomdb = factory_create_atomdb(atomdb_type);

    auto worker = [&](int tid, shared_ptr<AtomDB> atomdb) {
        if (action == "AddAtom") {
            add_atom(tid, atomdb, iterations);
        } else if (action == "AddAtoms") {
            add_atoms(tid, atomdb, iterations);
        } else if (action == "GetAtom") {
            get_atom(tid, atomdb, iterations);
        } else if (action == "GetAtoms") {
            get_atoms(tid, atomdb, iterations);
        } else if (action == "DeleteAtom") {
            delete_atom(tid, atomdb, iterations);
        } else if (action == "DeleteAtoms") {
            delete_atoms(tid, atomdb, iterations);
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

    create_report(get_type_name(*atomdb), action, global_latencies);

    return 0;
}