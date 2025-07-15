#include <cxxabi.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include "Logger.h"
#include "Utils.h"

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
void create_report(const string& db_name,
                   const string& action,
                   map<string, Metrics>& metrics,
                   const string& base_directory,
                   const int& batch_size);

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
