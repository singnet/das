#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
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
const int BATCH_SIZE = 100;  // Number of atoms in a single batch

mutex global_mutex;
map<string, Metrics> global_metrics;

void add_atom(int tid, shared_ptr<AtomDB> db, int iterations) {
    vector<double> local_add_node_operation_time;
    vector<double> local_add_link_operation_time;
    vector<double> local_add_atom_operation_time;

    local_add_node_operation_time.reserve(iterations);
    local_add_link_operation_time.reserve(iterations);
    local_add_atom_operation_time.reserve(iterations);

    // add_nodes
    for (int i = 0; i < iterations; ++i) {
        auto node_a = new Node(
            "Symbol", string("\"add_node_a") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");

        auto t0 = Clock::now();
        auto node_a_handle = db->add_node(node_a);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_add_node_operation_time.push_back(ms);
    }
    double nodes_total_time =
        std::accumulate(local_add_node_operation_time.begin(), local_add_node_operation_time.end(), 0.0);

    // add_links
    for (int i = 0; i < iterations; i++) {
        auto node_equivalence =
            new Node("Symbol", string("EQUIVALENCE_A") + "_t" + to_string(tid) + "_i" + to_string(i));
        auto node_equivalence_handle = db->add_node(node_equivalence);
        auto node_a = new Node(
            "Symbol", string("\"add_node_b") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_a_handle = db->add_node(node_a);
        auto node_b = new Node(
            "Symbol", string("\"add_node_c") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_b_handle = db->add_node(node_b);
        auto link = new Link("Expression", {node_equivalence_handle, node_a_handle, node_b_handle});

        auto t0 = Clock::now();
        auto link_handle = db->add_link(link);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_add_link_operation_time.push_back(ms);
    }
    double links_total_time =
        std::accumulate(local_add_link_operation_time.begin(), local_add_link_operation_time.end(), 0.0);

    // add_atoms
    for (int i = 0; i < iterations; ++i) {
        auto node_d = new Node(
            "Symbol", string("\"add_node_d") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");

        auto t0 = Clock::now();
        auto node_d_handle = db->add_atom(node_d);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_add_atom_operation_time.push_back(ms);
    }
    double atoms_total_time =
        std::accumulate(local_add_atom_operation_time.begin(), local_add_atom_operation_time.end(), 0.0);

    lock_guard<mutex> lock(global_mutex);
    global_metrics["add_node"] = Metrics{local_add_node_operation_time,
                                         nodes_total_time,
                                         nodes_total_time / iterations,
                                         iterations / (nodes_total_time / 1000.0)};
    global_metrics["add_link"] = Metrics{local_add_link_operation_time,
                                         links_total_time,
                                         links_total_time / iterations,
                                         iterations / (links_total_time / 1000.0)};
    global_metrics["add_atom"] = Metrics{local_add_atom_operation_time,
                                         atoms_total_time,
                                         atoms_total_time / iterations,
                                         iterations / (atoms_total_time / 1000.0)};
}

void add_atoms(int tid, shared_ptr<AtomDB> db, int iterations) {
    vector<double> local_add_nodes_operation_time;
    vector<double> local_add_links_operation_time;
    vector<double> local_add_atoms_operation_time;

    local_add_nodes_operation_time.reserve(iterations);
    local_add_links_operation_time.reserve(iterations);
    local_add_atoms_operation_time.reserve(iterations);

    // Add nodes
    for (int i = 0; i < iterations; ++i) {
        vector<Node*> nodes;

        // LOG_DEBUG("Thread_ID: " + to_string(tid) + " - Iteration: " + to_string(i));

        for (int j = 0; j < BATCH_SIZE; j++) {
            nodes.push_back(new Node("Symbol",
                                     string("\"add_nodes_a") + "_t" + to_string(tid) + "_i" +
                                         to_string(i) + to_string(j) + "\""));
        }

        auto t0 = Clock::now();
        // LOG_DEBUG("Adding nodes in batch of " + to_string(BATCH_SIZE) + " for thread " +
        // to_string(tid));
        db->add_nodes(nodes);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_add_nodes_operation_time.push_back(ms);
    }
    double nodes_total_time = std::accumulate(
        local_add_nodes_operation_time.begin(), local_add_nodes_operation_time.end(), 0.0);

    // LOG_DEBUG("Finished adding nodes in " + to_string(nodes_total_time) + " ms");

    // Add links
    for (int i = 0; i < iterations; i++) {
        vector<Link*> links;

        for (int j = 0; j < BATCH_SIZE; j++) {
            string suffix = "_t" + to_string(tid) + "_i" + to_string(i) + to_string(j);
            auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_B") + suffix);
            auto node_equivalence_handle = db->add_node(node_equivalence);
            auto node_a = new Node("Symbol", string("\"add_nodes_b") + suffix + "\"");
            auto node_a_handle = db->add_node(node_a);
            auto node_b = new Node("Symbol", string("\"add_nodes_c") + suffix + "\"");
            auto node_b_handle = db->add_node(node_b);
            links.push_back(
                new Link("Expression", {node_equivalence_handle, node_a_handle, node_b_handle}));
        }

        auto t0 = Clock::now();
        db->add_links(links);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_add_links_operation_time.push_back(ms);
    }
    double links_total_time = std::accumulate(
        local_add_links_operation_time.begin(), local_add_links_operation_time.end(), 0.0);

    // LOG_DEBUG("Finished adding links in " + to_string(links_total_time) + " ms");

    // Add atoms
    for (int i = 0; i < iterations; ++i) {
        vector<Atom*> atoms;

        for (int j = 0; j < BATCH_SIZE; j++) {
            atoms.push_back(new Node("Symbol",
                                     string("\"add_nodes_d") + "_t" + to_string(tid) + "_i" +
                                         to_string(i) + to_string(j) + "\""));
        }

        auto t0 = Clock::now();
        db->add_atoms(atoms);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_add_atoms_operation_time.push_back(ms);
    }
    double atoms_total_time = std::accumulate(
        local_add_atoms_operation_time.begin(), local_add_atoms_operation_time.end(), 0.0);

    // LOG_DEBUG("Finished adding atoms in " + to_string(atoms_total_time) + " ms");

    lock_guard<mutex> lock(global_mutex);
    global_metrics["add_nodes"] = Metrics{local_add_nodes_operation_time,
                                          nodes_total_time,
                                          nodes_total_time / (iterations * BATCH_SIZE),
                                          (iterations * BATCH_SIZE) / (nodes_total_time / 1000.0)};
    global_metrics["add_links"] = Metrics{local_add_links_operation_time,
                                          links_total_time,
                                          links_total_time / (iterations * BATCH_SIZE),
                                          (iterations * BATCH_SIZE) / (links_total_time / 1000.0)};
    global_metrics["add_atoms"] = Metrics{local_add_atoms_operation_time,
                                          atoms_total_time,
                                          atoms_total_time / (iterations * BATCH_SIZE),
                                          (iterations * BATCH_SIZE) / (atoms_total_time / 1000.0)};
}

void get_atom(int tid, shared_ptr<AtomDB> db, int iterations) {
    vector<double> local_get_node_document_operation_time;
    vector<double> local_get_link_document_operation_time;
    vector<double> local_get_atom_document_operation_time;
    vector<double> local_get_atom_operation_time;

    local_get_node_document_operation_time.reserve(iterations);
    local_get_link_document_operation_time.reserve(iterations);
    local_get_atom_document_operation_time.reserve(iterations);
    local_get_atom_operation_time.reserve(iterations);

    // get_node_document
    for (int i = 0; i < iterations; ++i) {
        auto node_a = new Node(
            "Symbol", string("\"get_atom_a") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_a_handle = db->add_node(node_a);

        auto t0 = Clock::now();
        db->get_node_document(node_a_handle);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_get_node_document_operation_time.push_back(ms);
    }
    double get_node_document_total_time = std::accumulate(local_get_node_document_operation_time.begin(),
                                                          local_get_node_document_operation_time.end(),
                                                          0.0);

    // get_link_document
    for (int i = 0; i < iterations; ++i) {
        auto node_equivalence =
            new Node("Symbol", string("EQUIVALENCE_C") + "_t" + to_string(tid) + "_i" + to_string(i));
        auto node_equivalence_handle = db->add_node(node_equivalence);
        auto node_b = new Node(
            "Symbol", string("\"get_atom_b") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_b_handle = db->add_node(node_b);
        auto node_c = new Node(
            "Symbol", string("\"get_atom_c") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_c_handle = db->add_node(node_c);

        auto link = new Link("Expression", {node_equivalence_handle, node_b_handle, node_c_handle});

        auto link_handle = db->add_link(link);

        auto t0 = Clock::now();
        db->get_link_document(link_handle);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_get_link_document_operation_time.push_back(ms);
    }
    double get_link_document_total_time = std::accumulate(local_get_link_document_operation_time.begin(),
                                                          local_get_link_document_operation_time.end(),
                                                          0.0);

    // get_atom_document
    for (int i = 0; i < iterations; ++i) {
        auto node_d = new Node(
            "Symbol", string("\"get_atom_d") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_d_handle = db->add_atom(node_d);

        auto t0 = Clock::now();
        db->get_atom_document(node_d_handle);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_get_atom_document_operation_time.push_back(ms);
    }
    double get_atom_document_total_time = std::accumulate(local_get_atom_document_operation_time.begin(),
                                                          local_get_atom_document_operation_time.end(),
                                                          0.0);

    // get_atom
    for (int i = 0; i < iterations; ++i) {
        auto node_e = new Node(
            "Symbol", string("\"get_atom_e") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_e_handle = db->add_atom(node_e);

        auto t0 = Clock::now();
        db->get_atom(node_e_handle);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        local_get_atom_operation_time.push_back(ms);
    }
    double get_atom_total_time =
        std::accumulate(local_get_atom_operation_time.begin(), local_get_atom_operation_time.end(), 0.0);

    lock_guard<mutex> lock(global_mutex);
    global_metrics["get_node_document"] = Metrics{local_get_node_document_operation_time,
                                                  get_node_document_total_time,
                                                  get_node_document_total_time / iterations,
                                                  iterations / (get_node_document_total_time / 1000.0)};
    global_metrics["get_link_document"] = Metrics{local_get_link_document_operation_time,
                                                  get_link_document_total_time,
                                                  get_link_document_total_time / iterations,
                                                  iterations / (get_link_document_total_time / 1000.0)};
    global_metrics["get_atom_document"] = Metrics{local_get_atom_document_operation_time,
                                                  get_atom_document_total_time,
                                                  get_atom_document_total_time / iterations,
                                                  iterations / (get_atom_document_total_time / 1000.0)};
    global_metrics["get_atom"] = Metrics{local_get_atom_operation_time,
                                         get_atom_total_time,
                                         get_atom_total_time / iterations,
                                         iterations / (get_atom_total_time / 1000.0)};
}

void get_atoms(int tid, shared_ptr<AtomDB> db, int iterations) { /* WIP */
}

void delete_atom(int tid, shared_ptr<AtomDB> db, int iterations) { /* WIP */
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
             << " <atomdb_type> <action> <cache> <num_concurrency> <num_iterations> <timestamp>" << endl;
        exit(1);
    }

    string atomdb_type = argv[1];
    string action = argv[2];
    bool cache_enable = ((string(argv[3]) == "enabled") ? true : false);
    int concurrency = stoi(argv[4]);
    int iterations = stoi(argv[5]);
    string timestamp = argv[6];

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

    string base_directory = "/tmp/atomdb_benchmark/" + timestamp;

    int batch_size = 1;
    if (action == "AddAtoms" || action == "DeleteAtoms" || action == "GetAtoms") {
        batch_size = BATCH_SIZE;
    }

    create_report(get_type_name(*atomdb), action, global_metrics, base_directory, batch_size);

    return 0;
}