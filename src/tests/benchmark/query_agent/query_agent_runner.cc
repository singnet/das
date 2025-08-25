#include "query_agent_runner.h"

const regex QueryAgentRunner::BENCHMARK_PATTERN(
    ".*STOP_WATCH \"Benchmark::PatternMatchingQuery\" (\\d+).*");

QueryAgentRunner::QueryAgentRunner(int tid, shared_ptr<AtomSpace> atom_space, int iterations)
    : Runner(tid, iterations), atom_space_(atom_space) {}

vector<double> QueryAgentRunner::parse_server_side_benchmark_times(const string& log_file) {
    vector<double> result;

    // Check if file exists and is accessible
    ifstream check_file(log_file);
    if (!check_file.good()) {
        cerr << "ERROR: File not found or cannot be opened: " << log_file << endl;
        return result;
    }
    check_file.close();

    // Process the file
    ifstream file(log_file);
    if (file.is_open()) {
        string line;
        smatch matches;
        while (getline(file, line)) {
            if (line.find("Benchmark::PatternMatchingQuery") != string::npos) {
                if (regex_search(line, matches, BENCHMARK_PATTERN)) {
                    try {
                        double value = stod(matches[1].str());
                        result.push_back(value);
                    } catch (const exception& e) {
                        cerr << "ERROR converting to number: " << matches[1].str() << endl;
                        cerr << e.what() << endl;
                    }
                }
            }
        }

        file.close();
    } else {
        cerr << "ERROR opening file (even after checking): " << log_file << endl;
    }

    return result;
}

bool QueryAgentRunner::parse_ms_from_line(const string& line, double& value) {
    const string pattern = "#PROFILER# STOP_WATCH \"Benchmark::PatternMatchingQuery\"";
    size_t pos = line.find(pattern);
    if (pos != string::npos) {
        string sub = line.substr(pos + pattern.length());
        stringstream ss(sub);
        ss >> value;
        return !ss.fail();
    }
    return false;
}