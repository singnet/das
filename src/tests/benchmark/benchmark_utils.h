#pragma once

#include <cxxabi.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <functional>
#include <map>
#include <string>
#include <typeinfo>
#include <vector>

using namespace std;

struct Metrics {
    vector<double> operation_time;
    double total_time;
    double time_per_atom;
    double throughput;
};

// Compute percentile (0.0 to 1.0) of the sorted vector using linear interpolation
double compute_percentile(const vector<double>& vec, double percentile_fraction);

// Compute metric statistics for each operation
map<string, map<string, double>> calculate_metric_statistics(map<string, Metrics>& metrics);

// Create a report for the given metrics
void create_report(string filename, map<string, Metrics>& metrics);

// Get demangled type name of an object
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

template <typename Func>
double measure_execution_time(Func&& func) {
    auto start = chrono::steady_clock::now();
    func();
    auto end = chrono::steady_clock::now();
    double duration_millis = chrono::duration<double, milli>(end - start).count();
    return duration_millis;
}

void dispatch_handler(const map<string, function<void()>>& handlers, const string& method);

// Check if the given directory has a log file
bool is_log_file(const string& directory);
