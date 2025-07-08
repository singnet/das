#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

using namespace std;


void set_cache(bool enabled) {
    // TODO
}

bool add_atom(int id) {
    // TODO
    return true;
}

bool add_atoms(int id) {
    // TODO
    return true;
}

bool get_atom(int id, bool very_connected) {
    // TODO
    return true;
}

bool get_atoms(int id, bool very_connected) {
    // TODO
    return true;
}

bool delete_atom(int id) {
    // TODO
    return true;
}

bool delete_atoms(int id) {
    // TODO
    return true;
}

struct Stats {
    atomic<size_t> total_ops{0};
    atomic<size_t> errors{0};
    atomic<double> total_latency_ms{0};
};

// -----------------------------------------------------
void worker_thread(const string& action, int iterations, bool very_connected, Stats& stats) {
    for (int i = 0; i < iterations; ++i) {
        auto start = chrono::high_resolution_clock::now();
        bool ok = false;

        if (action == "add_atom") {
            ok = add_atom(i);
        } else if (action == "get_atom") {
            ok = get_atom(i, very_connected);
        } else if (action == "delete_atom") {
            ok = delete_atom(i);
        } else {
            cerr << "Unknown action: " << action << endl;
            continue;
        }

        auto end = chrono::high_resolution_clock::now();
        double ms = chrono::duration<double, milli>(end - start).count();

        // stats.total_latency_ms += ms;
        if (!ok) stats.errors++;
        stats.total_ops++;
    }
}

// -----------
int main(){};