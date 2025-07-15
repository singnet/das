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

map<string, map<string, double>> calculate_metric_statistics(map<string, Metrics>& metrics) {
    map<string, map<string, double>> stats;

    for (auto& [operation, metric] : metrics) {
        if (metric.operation_time.empty()) {
            Utils::error("metrics for operation '" + operation + "' are empty");
        }

        double sum = accumulate(metric.operation_time.begin(), metric.operation_time.end(), 0.0);
        double mean = sum / metric.operation_time.size();

        sort(metric.operation_time.begin(), metric.operation_time.end());

        double min_ms = metric.operation_time.front();
        double max_ms = metric.operation_time.back();
        double p50 = compute_percentile(metric.operation_time, 0.50);
        double p90 = compute_percentile(metric.operation_time, 0.90);
        double p99 = compute_percentile(metric.operation_time, 0.99);

        stats[operation] = {{"mean_operation_time", mean},
                            {"min_operation_time", min_ms},
                            {"max_operation_time", max_ms},
                            {"p50_operation_time", p50},
                            {"p90_operation_time", p90},
                            {"p99_operation_time", p99},
                            {"total_time", metric.total_time},
                            {"time_per_atom", metric.time_per_atom},
                            {"throughput", metric.throughput}};
    }
    return stats;
}

void create_report(const string& db_name,
                   const string& action,
                   map<string, Metrics>& metrics,
                   const string& base_directory,
                   const int& batch_size) {
    stringstream table;
    table << fixed << setprecision(5);
    table << left;

    const int col1 = 15;
    const int colN = 13;
    table << setw(col1) << "Operation"
          << "| " << setw(colN) << "AVG"
          << "| " << setw(colN) << "MIN"
          << "| " << setw(colN) << "MAX"
          << "| " << setw(colN) << "P50"
          << "| " << setw(colN) << "P90"
          << "| " << setw(colN) << "P99"
          << "| " << setw(colN) << "TT"
          << "| " << setw(colN) << "TPA"
          << "| " << setw(colN) << "TP"
          << "\n";

    for (int i = 0; i < col1; ++i) table << '-';
    table << "| ";
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < colN; ++j) table << '-';
        table << "| ";
    }
    table << "\n";

    map<string, map<string, double>> stats = calculate_metric_statistics(metrics);

    for (const auto& [operation, inner_map] : stats) {
        table << setw(col1) << operation << "| " << setw(colN) << inner_map.at("mean_operation_time")
              << "| " << setw(colN) << inner_map.at("min_operation_time") << "| " << setw(colN)
              << inner_map.at("max_operation_time") << "| " << setw(colN)
              << inner_map.at("p50_operation_time") << "| " << setw(colN)
              << inner_map.at("p90_operation_time") << "| " << setw(colN)
              << inner_map.at("p99_operation_time") << "| " << setw(colN) << inner_map.at("total_time")
              << "| " << setw(colN) << inner_map.at("time_per_atom") << "| " << setw(colN)
              << inner_map.at("throughput") << "| "
              << "\n";
    }
    string filename =
        base_directory + "/" + db_name + "_" + action + "_" + to_string(batch_size) + ".txt";

    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << table.str();
        outfile.close();
        LOG_DEBUG("Done. Check the report file for more details: " + filename);
    } else {
        cerr << "error: " << filename << endl;
    }
}
