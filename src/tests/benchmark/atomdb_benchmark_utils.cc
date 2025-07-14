#include "atomdb_benchmark_utils.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>

#include "Logger.h"
#include "Utils.h"

#define LOG_LEVEL DEBUG_LEVEL

using namespace std;
using namespace commons;

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

map<string, map<string, double>> latency_statistics(map<string, vector<double>>& latencies) {
    map<string, map<string, double>> stats;
    for (auto& [operation, vec] : latencies) {
        if (vec.empty()) {
            Utils::error("latencies for operation '" + operation + "' are empty");
        }

        double sum = accumulate(vec.begin(), vec.end(), 0.0);
        double mean = sum / vec.size();

        sort(vec.begin(), vec.end());

        double min_ms = vec.front();
        double max_ms = vec.back();
        double p50 = compute_percentile(vec, 0.50);
        double p90 = compute_percentile(vec, 0.90);
        double p99 = compute_percentile(vec, 0.99);

        stats[operation] = {
            {"mean", mean}, {"min", min_ms}, {"max", max_ms}, {"p50", p50}, {"p90", p90}, {"p99", p99}};
    }
    return stats;
}

void create_report(const string& db_name, const string& action, map<string, vector<double>>& latencies) {
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

    map<string, map<string, double>> stats = latency_statistics(latencies);

    for (const auto& [operation, inner_map] : stats) {
        table << setw(col1) << operation << "| " << setw(colN) << inner_map.at("mean") << "| "
              << setw(colN) << inner_map.at("min") << "| " << setw(colN) << inner_map.at("max") << "| "
              << setw(colN) << inner_map.at("p50") << "| " << setw(colN) << inner_map.at("p90") << "| "
              << setw(colN) << inner_map.at("p99") << "| "
              << "\n";
    }
    string filename = "/tmp/atomdb_benchmark/" + db_name + "_" + action + ".txt";

    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << table.str();
        outfile.close();
        LOG_DEBUG("Done. Check the report file for more details: " + filename);
    } else {
        cerr << "error: " << filename << endl;
    }
}
