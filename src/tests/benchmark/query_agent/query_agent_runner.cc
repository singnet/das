#include "query_agent_runner.h"

QueryAgentRunner::QueryAgentRunner(int tid, shared_ptr<AtomSpace> atom_space, int iterations)
    : Runner(tid, iterations), atom_space_(atom_space) {}

vector<double> QueryAgentRunner::parse_server_side_benchmark_times(const string& log_file) {
    vector<double> result;

    // Check if file exists and is accessible
    ifstream check_file(log_file);
    if (!check_file.good()) {
        cerr << "ERROR: File not found or cannot be opened: " << log_file << endl;
        return result;  // Return empty vector
    }
    check_file.close();

    // Process the file
    ifstream file(log_file);
    if (file.is_open()) {
        string line;
        regex pattern(".*STOP_WATCH \"Benchmark::PatternMatchingQuery\" (\\d+).*");
        smatch matches;

        while (getline(file, line)) {
            // Quick pre-check to avoid regex on every line
            if (line.find("STOP_WATCH") != string::npos &&
                line.find("Benchmark::PatternMatchingQuery") != string::npos) {
                if (regex_search(line, matches, pattern)) {
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