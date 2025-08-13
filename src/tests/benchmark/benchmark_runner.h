#pragma once

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "benchmark_utils.h"

using namespace std;

extern mutex global_mutex;
extern map<string, Metrics> global_metrics;

/**
 * @brief Runner is a generic benchmarking base class for AtomDB operations.
 *
 * This class provides utilities for running timed benchmarks on AtomDB operations,
 * collecting performance metrics, and sampling handles from link sets.
 *
 */
class Runner {
   public:
    /**
     * @brief Construct a new Runner object.
     *
     * @param tid        Thread ID for this runner instance.
     * @param db         Shared pointer to the AtomDB instance.
     * @param iterations Number of benchmark iterations to execute.
     */
    Runner(int tid, int iterations);

    /**
     * Desctructor.
     */
    virtual ~Runner() {}

   protected:
    int tid_;
    int iterations_;
    /**
     * @brief Maximum count for random link handle selection.
     */
    static const size_t MAX_COUNT;  // Maximum count for random link handle selection
    /**
     * @brief Link query pattern for "contains" relationships.
     */
    static const vector<string> contains_links_query;
    /**
     * @brief Link query pattern for "sentence" relationships.
     */
    static const vector<string> sentence_links_query;

    /**
     * @brief Runs a benchmark for a given operation.
     *
     * Executes a setup function and a benchmark function for a specified number of iterations,
     * measures the execution time for each operation, and records metrics in the global metrics map.
     *
     * @param key      A string key identifying the benchmark operation.
     * @param setup    A function/lambda to set up the data for each iteration.
     * @param bench    A function/lambda that executes the operation to be measured.
     * @param divisor  Optional: Divides the number of operations for averaging purposes (default: 1).
     */
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
};
