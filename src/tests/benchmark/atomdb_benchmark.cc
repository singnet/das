#include <atomic>
#include <chrono>
#include <iostream>
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
static atomic<uint64_t> global_id_counter{0};


template <typename DB>
void add_atom(shared_ptr<DB> db, const string& db_name, int iterations) {
    vector<double> latencies;
    latencies.reserve(iterations);

    auto t_add_atom_start = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        uint64_t unique_id = global_id_counter.fetch_add(1, memory_order_relaxed);

        auto t0 = Clock::now();

        string node_equivalence_name = string("EQUIVALENCE") + "_i" + to_string(i) + "_u" + to_string(unique_id);
        cout << "node_equivalence_name: " << node_equivalence_name << endl;
        auto node_equivalence = new Node("Symbol", node_equivalence_name);
        db->add_node(node_equivalence);
        
        string node_a_name = string("\"add_node_a") + "_i" + to_string(i) + "_u" + to_string(unique_id) + "\"";
        cout << "node_a_name: " << node_a_name << endl;
        auto node_a = new Node("Symbol", node_a_name);
        db->add_node(node_a);
        
        string node_b_name = string("\"add_node_b") + "_i" + to_string(i) + "_u" + to_string(unique_id) + "\"";
        cout << "node_b_name: " << node_b_name << endl;
        auto node_b = new Node("Symbol", node_b_name);
        db->add_node(node_b);
        
        auto link = new Link("Expression", {node_equivalence->handle(), node_a->handle(), node_b->handle()});
        cout << "link_handle: " << link->handle() << endl;
        
        db->add_link(link);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        latencies.push_back(ms);
    }
    auto t_add_atom_end = Clock::now();

    double sum = accumulate(latencies.begin(), latencies.end(), 0.0);
    double mean = sum / latencies.size();

    sort(latencies.begin(), latencies.end());

    // Calculates the percentile (0.0 to 1.0) of the sorted vector of latencies using linear interpolation.
    // Errors out if p is outside the [0,1] range.
    auto percentile = [&](double percentile_fraction) {
        if (percentile_fraction < 0.0 || percentile_fraction > 1.0) {
            Utils::error("p out of range");
        }

        size_t latencies_size = latencies.size();
        double exact_position = percentile_fraction * (latencies_size - 1);
        size_t lower_index = static_cast<size_t>(floor(exact_position));
        double interpolation_weight = exact_position - lower_index;

        // If a next element exists, interpolate between the two surrounding values
        if (lower_index + 1 < latencies_size)
            return latencies[lower_index] +
                   interpolation_weight * (latencies[lower_index + 1] - latencies[lower_index]);
        else
            return latencies[lower_index];  // Edge case
    };

    double p50 = percentile(0.50);
    double p90 = percentile(0.90);
    double p99 = percentile(0.99);

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
void add_atoms(atomdb_api_types::ATOMDB_TYPE type, const string& db_name, int iterations) {
    // WIP
}

template <typename DB>
void get_atom(atomdb_api_types::ATOMDB_TYPE type, const string& db_name, int iterations) {
    // WIP
}

template <typename DB>
void get_atoms(atomdb_api_types::ATOMDB_TYPE type, const string& db_name, int iterations) {
    // WIP
}

template <typename DB>
void delete_atom(atomdb_api_types::ATOMDB_TYPE type, const string& db_name, int iterations) {
    // WIP
}

template <typename DB>
void delete_atoms(atomdb_api_types::ATOMDB_TYPE type, const string& db_name, int iterations) {
    // WIP
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
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <action> <cache:enabled|disabled> <num_concurrency> <num_iterations>" << endl;
        exit(1);
    }

    vector<string> actions = Utils::split(argv[1], ' ');
    bool cache_enable = ((string(argv[2]) == "enabled") ? true : false);
    int concurrency = stoi(argv[3]);
    int iterations = stoi(argv[4]);

    setup(cache_enable);

    // AtomDBSingleton::reset();
    // AtomDBCacheSingleton::reset();
    AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::REDIS_MONGODB);
    auto base = AtomDBSingleton::get_instance();
    auto db = dynamic_pointer_cast<RedisMongoDB>(base);

    std::mutex mtx;
    

    atomic<size_t> counter{0};
    for (const auto& action : actions) {
        auto worker = [&](int tid, shared_ptr<RedisMongoDB> db_ptr) {
            if (action == "AddAtom") {
                // std::lock_guard<std::mutex> lock(mtx);
                add_atom<RedisMongoDB>(db_ptr, "RedisMongoDB", iterations);
            } else if (action == "AddAtoms") {
                add_atoms<RedisMongoDB>(atomdb_api_types::ATOMDB_TYPE::REDIS_MONGODB, "RedisMongoDB", iterations);
            } else if (action == "GetAtom") {
                get_atom<RedisMongoDB>(atomdb_api_types::ATOMDB_TYPE::REDIS_MONGODB, "RedisMongoDB", iterations);
                get_atom<MorkDB>(atomdb_api_types::ATOMDB_TYPE::MORKDB, "MorkDB", iterations);
            } else if (action == "GetAtoms") {
                get_atoms<RedisMongoDB>(atomdb_api_types::ATOMDB_TYPE::REDIS_MONGODB, "RedisMongoDB", iterations);
                get_atoms<MorkDB>(atomdb_api_types::ATOMDB_TYPE::MORKDB, "MorkDB", iterations);
            } else if (action == "DeleteAtom") {
                delete_atom<RedisMongoDB>(atomdb_api_types::ATOMDB_TYPE::REDIS_MONGODB, "RedisMongoDB", iterations);
            } else if (action == "DeleteAtoms") {
                delete_atoms<RedisMongoDB>(atomdb_api_types::ATOMDB_TYPE::REDIS_MONGODB, "RedisMongoDB", iterations);
            } else {
                Utils::error(
                    "Invalid action. Choose either 'AddAtom' or 'AddAtoms' or 'GetAtom' or 'GetAtoms' "
                    "or 'DeleteAtom' or 'DeleteAtoms'");
            }
        };

        vector<thread> threads;

        auto t_start = Clock::now();
        for (int t = 0; t < concurrency; ++t) {
            threads.emplace_back(worker, t, db);
        }
        for (auto& th : threads) th.join();
        auto t_end = Clock::now();

        ++counter;
    }

    
    return 0;
}
