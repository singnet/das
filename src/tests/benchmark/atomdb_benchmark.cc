#include <cxxabi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "AtomDBSingleton.h"
#include "Logger.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL

using namespace std;
using namespace atomdb;

using Clock = chrono::steady_clock;

// Calculates the percentile (0.0 to 1.0) of the sorted vector using linear interpolation.
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
        return vec[lower_index] + interpolation_weight * (vec[lower_index + 1] - vec[lower_index]);
    else
        return vec[lower_index];  // Edge case
};

map<string, double> latency_statistics(vector<double>& latencies) {
    if (latencies.empty()) {
        Utils::error("latencies vector is empty");
    }
    double sum = accumulate(latencies.begin(), latencies.end(), 0.0);
    double mean = sum / latencies.size();

    sort(latencies.begin(), latencies.end());

    double min_ms = latencies.front();
    double max_ms = latencies.back();
    double p50 = compute_percentile(latencies, 0.50);
    double p90 = compute_percentile(latencies, 0.90);
    double p99 = compute_percentile(latencies, 0.99);

    return {{"mean", mean}, {"min", min_ms}, {"max", max_ms}, {"p50", p50}, {"p90", p90}, {"p99", p99}};
}

void create_latency_report(map<string, double>& node_stats,
                           map<string, double>& link_stats,
                           string& db_name,
                           int tid) {
    stringstream table;
    table << fixed << setprecision(5);
    table << left;

    const int col1 = 12;
    const int colN = 10;
    table << setw(col1) << "Operation"
          << "| " << setw(colN) << "Mean (ms)"
          << "| " << setw(colN) << "Min (ms)"
          << "| " << setw(colN) << "Max (ms)"
          << "| " << setw(colN) << "P50 (ms)"
          << "| " << setw(colN) << "P90 (ms)"
          << "| " << setw(colN) << "P99 (ms)"
          << "| "
          << "\n";

    for (int i = 0; i < col1; ++i) table << '-';
    table << "| ";
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < colN; ++j) table << '-';
        table << "| ";
    }
    table << "\n";

    table << setw(col1) << "add_node"
          << "| " << setw(colN) << node_stats["mean"] << "| " << setw(colN) << node_stats["min"] << "| "
          << setw(colN) << node_stats["max"] << "| " << setw(colN) << node_stats["p50"] << "| "
          << setw(colN) << node_stats["p90"] << "| " << setw(colN) << node_stats["p99"] << "| "
          << "\n";

    table << setw(col1) << "add_link"
          << "| " << setw(colN) << link_stats["mean"] << "| " << setw(colN) << link_stats["min"] << "| "
          << setw(colN) << link_stats["max"] << "| " << setw(colN) << link_stats["p50"] << "| "
          << setw(colN) << link_stats["p90"] << "| " << setw(colN) << link_stats["p99"] << "| "
          << "\n";

    string filename = "/tmp/benchmark_add_atom_" + db_name + "_thread_" + to_string(tid) + ".txt";

    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << table.str();
        outfile.close();
        LOG_INFO("Done. Check the report file for more details: " + filename);
    } else {
        cerr << "error: " << filename << endl;
    }
}

template <typename T>
string get_type_name(const T& obj) {
    const char* mangled = typeid(obj).name();
    int status;
    char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    string result = (status == 0 && demangled != nullptr) ? demangled : mangled;
    free(demangled);

    // sanitize
    size_t pos = 0;
    while ((pos = result.find("::", pos)) != string::npos) {
        result.replace(pos, 2, "_");
        pos += 1;
    }
    transform(result.begin(), result.end(), result.begin(), ::tolower);

    return result;
}

void add_atom(int tid, shared_ptr<AtomDB> db, int iterations) {
    vector<double> latencies;
    latencies.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        auto node_a = new Node(
            "Symbol", string("\"add_node_a") + "_t" + to_string(tid) + "_i" + to_string(i) + "\"");

        auto t0 = Clock::now();
        db->add_node(node_a);
        auto t1 = Clock::now();

        double ms = chrono::duration<double, milli>(t1 - t0).count();
        latencies.push_back(ms);
    }
    auto node_stats = latency_statistics(latencies);

    latencies.clear();
    latencies.reserve(iterations);

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
        latencies.push_back(ms);
    }
    auto link_stats = latency_statistics(latencies);

    string db_name = get_type_name(*db);

    create_latency_report(node_stats, link_stats, db_name, tid);
}

void add_atoms(int tid, shared_ptr<AtomDB> db, int iterations) {
    // WIP
}

void get_atom(int tid, shared_ptr<AtomDB> db, int iterations) {
    // WIP
}

void get_atoms(int tid, shared_ptr<AtomDB> db, int iterations) {
    // WIP
}

void delete_atom(int tid, shared_ptr<AtomDB> db, int iterations) {
    // WIP
}

void delete_atoms(int tid, shared_ptr<AtomDB> db, int iterations) {
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
    // auto t_start = Clock::now();
    for (auto& th : threads) {
        th.join();
    }
    // auto t_end = Clock::now();

    return 0;
}
