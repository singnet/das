#pragma once

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "atomdb_utils.h"

using namespace std;
using namespace atomdb;

extern mutex global_mutex;
extern map<string, Metrics> global_metrics;

template <typename T>
class AtomDBRunner {
   public:
    AtomDBRunner(int tid, shared_ptr<AtomDB> db, int iterations)
        : tid_(tid), db_(db), iterations_(iterations) {}

   protected:
    int tid_;
    shared_ptr<AtomDB> db_;
    int iterations_;
    static const size_t MAX_COUNT;  // Maximum count for random link handle selection
    static const vector<string> contains_links_query;
    static const vector<string> sentence_links_query;

    template <typename Fsetup, typename Fbench>
    void run_benchmark(const string& key, Fsetup&& setup, Fbench&& bench, int divisor = 1) {
        vector<double> operation_time;
        for (int i = 0; i < iterations_; ++i) {
            auto ret = setup(i);
            double duration = measure_execution_time([&] { bench(ret); });
            operation_time.push_back(duration);
        }
        double total_time = accumulate(operation_time.begin(), operation_time.end(), 0.0);
        int total_ops = iterations_ * divisor;
        double avg_time = total_time / total_ops;
        double ops_per_sec = total_ops / (total_time / 1000.0);

        global_mutex.lock();
        global_metrics[key] = Metrics{operation_time, total_time, avg_time, ops_per_sec};
        global_mutex.unlock();
    }

    template <typename HandleSet>
    vector<string> get_random_link_handle(const HandleSet& handle_set,
                                          size_t max_count = MAX_COUNT,
                                          size_t n = 1) {
        vector<char*> link_handles;

        auto it = handle_set->get_iterator();

        char* handle = nullptr;
        while ((handle = it->next()) != nullptr && (link_handles.size() < max_count)) {
            link_handles.push_back(handle);
        }

        if (link_handles.empty()) {
            return {};
        }

        auto handles_to_return = min(n, link_handles.size());

        vector<string> handles;
        for (size_t i = 0; i < handles_to_return; ++i) {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<size_t> dist(0, link_handles.size() - 1);
            auto h = link_handles[dist(gen)];
            handles.push_back(string(h));
        }
        return handles;
    }
};

template <typename T>
const size_t AtomDBRunner<T>::MAX_COUNT = 1000;

template <typename T>
const vector<string> AtomDBRunner<T>::contains_links_query = {"LINK_TEMPLATE",
                                                              "Expression",
                                                              "3",
                                                              "NODE",
                                                              "Symbol",
                                                              "Contains",
                                                              "VARIABLE",
                                                              "sentence",
                                                              "VARIABLE",
                                                              "word"};

template <typename T>
const vector<string> AtomDBRunner<T>::sentence_links_query = {
    "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "sentence"};