#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "AtomDBSingleton.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;

using Clock = chrono::high_resolution_clock;

// Calculates the percentile (0.0 to 1.0) of the sorted vector of latencies using linear
// interpolation. Errors out if p is outside the [0,1] range.
double compute_percentile(const vector<double>& vec, double percentile_fraction) {
    if (vec.empty()) {
        Utils::error("vector is empty");
    }
    if (percentile_fraction < 0.0 || percentile_fraction > 1.0) {
        Utils::error("percentile fraction out of range");
    }

    size_t vec_size = vec.size();
    double exact_position = percentile_fraction * (vec_size - 1);
    size_t lower_index = static_cast<size_t>(floor(exact_position));
    double interpolation_weight = exact_position - lower_index;

    // If a next element exists, interpolate between the two surrounding values
    if (lower_index + 1 < vec_size)
        return vec[lower_index] +
                interpolation_weight * (vec[lower_index + 1] - vec[lower_index]);
    else
        return vec[lower_index];  // Edge case
};


template <typename DB>
void add_atom(int tid, shared_ptr<DB> db, const string& db_name, int iterations) {
    vector<double> latencies;
    latencies.reserve(iterations);

    auto t_add_atom_start = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto t0 = Clock::now();

        auto node_equivalence =
            new Node("Symbol", string("EQUIVALENCE") + "_t" + to_string(tid) + "_i" + to_string(i));
        auto node_equivalence_handle = db->add_node(node_equivalence);

        auto node_a = new Node(
            "Symbol", string("\"add_node_a") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_a_handle = db->add_node(node_a);

        auto node_b = new Node(
            "Symbol", string("\"add_node_b") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");
        auto node_b_handle = db->add_node(node_b);

        // auto link = new Link("Expression", {node_equivalence_handle, node_a_handle, node_b_handle});
        // db->add_link(link);

        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        latencies.push_back(ms);
    }
    auto t_add_atom_end = Clock::now();

    double sum = accumulate(latencies.begin(), latencies.end(), 0.0);
    double mean = sum / latencies.size();

    sort(latencies.begin(), latencies.end());

    double p50 = compute_percentile(latencies, 0.50);
    double p90 = compute_percentile(latencies, 0.90);
    double p99 = compute_percentile(latencies, 0.99);

    double min_ms = latencies.front();
    double max_ms = latencies.back();

    cout << "Iteractions: " << iterations << "\n"
         << "Mean: " << mean << " ms\n"
         << "Min: " << min_ms << " ms\n"
         << "Max: " << max_ms << " ms\n"
         << "P50: " << p50 << " ms\n"
         << "P90: " << p90 << " ms\n"
         << "P99: " << p99 << " ms\n";
}

template <typename DB>
void add_atoms(int tid, shared_ptr<DB> db, const string& db_name, int iterations) {
    // WIP
}

template <typename DB>
void get_atom(int tid, shared_ptr<DB> db, const string& db_name, int iterations) {
    // WIP
}

template <typename DB>
void get_atoms(int tid, shared_ptr<DB> db, const string& db_name, int iterations) {
    // WIP
}

template <typename DB>
void delete_atom(int tid, shared_ptr<DB> db, const string& db_name, int iterations) {
    // WIP
}

template <typename DB>
void delete_atoms(int tid, shared_ptr<DB> db, const string& db_name, int iterations) {
    // WIP
}

void setup_environments(bool cache_enable) {
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
}

void init() {
    AtomDBCacheSingleton::init();
    RedisMongoDB::initialize_statics();
}


int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <action> <cache:enabled|disabled> <num_concurrency> <num_iterations>" << endl;
        exit(1);
    }

    string action = argv[1];
    bool cache_enable = ((string(argv[2]) == "enabled") ? true : false);
    int concurrency = stoi(argv[3]);
    int iterations = stoi(argv[4]);

    setup_environments(cache_enable);
    init();

    auto atomdb_redis_mongo = make_shared<RedisMongoDB>();
    auto atomdb_mork = make_shared<MorkDB>();

    auto worker = [&](int tid, shared_ptr<RedisMongoDB> atomdb_redis_mongo, shared_ptr<MorkDB> atomdb_mork) {
        if (action == "AddAtom") {
            add_atom<RedisMongoDB>(tid, atomdb_redis_mongo, "RedisMongoDB", iterations);
        } else if (action == "AddAtoms") {
            add_atoms<RedisMongoDB>(tid, atomdb_redis_mongo, "RedisMongoDB", iterations);
        } else if (action == "GetAtom") {
            get_atom<RedisMongoDB>(tid, atomdb_redis_mongo, "RedisMongoDB", iterations);
            get_atom<MorkDB>(tid, atomdb_mork, "MorkDB", iterations);
        } else if (action == "GetAtoms") {
            get_atoms<RedisMongoDB>(tid, atomdb_redis_mongo, "RedisMongoDB", iterations);
            get_atoms<MorkDB>(tid, atomdb_mork, "MorkDB", iterations);
        } else if (action == "DeleteAtom") {
            delete_atom<RedisMongoDB>(tid, atomdb_redis_mongo, "RedisMongoDB", iterations);
        } else if (action == "DeleteAtoms") {
            delete_atoms<RedisMongoDB>(tid, atomdb_redis_mongo, "RedisMongoDB", iterations);
        } else {
            Utils::error(
                "Invalid action. Choose either 'AddAtom' or 'AddAtoms' or 'GetAtom' or 'GetAtoms' "
                "or 'DeleteAtom' or 'DeleteAtoms'");
        }
    };

    vector<thread> threads;

    auto t_start = Clock::now();
    for (int t = 0; t < concurrency; t++) {
        threads.emplace_back(worker, t, atomdb_redis_mongo, atomdb_mork);
    }
    for (auto& th : threads) th.join();
    auto t_end = Clock::now();

    return 0;
}
