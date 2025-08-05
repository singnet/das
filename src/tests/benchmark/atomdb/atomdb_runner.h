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
#include "MorkDB.h"
#include "atomdb_utils.h"

using namespace std;
using namespace atomdb;

extern mutex global_mutex;
extern map<string, Metrics> global_metrics;

/**
 * @brief AtomDBRunner is a generic benchmarking base class for AtomDB operations.
 *
 * This class provides utilities for running timed benchmarks on AtomDB operations,
 * collecting performance metrics, and sampling handles from link sets.
 *
 * @tparam T The derived class type, used for inheritance.
 */
template <typename T>
class AtomDBRunner {
   public:
    /**
     * @brief Construct a new AtomDBRunner object.
     *
     * @param tid        Thread ID for this runner instance.
     * @param db         Shared pointer to the AtomDB instance.
     * @param iterations Number of benchmark iterations to execute.
     */
    AtomDBRunner(int tid, shared_ptr<AtomDB> db, int iterations)
        : tid_(tid), db_(db), iterations_(iterations) {}

   protected:
    int tid_;
    shared_ptr<AtomDB> db_;
    int iterations_;
    static const size_t MAX_COUNT;  // Maximum count for random link handle selection
    static const vector<string> contains_links_query;
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

    /**
     * @brief Selects random link handles from a given handle set.
     *
     * Iterates over handle_set and randomly selects up to n handles (not exceeding max_count).
     *
     * @tparam HandleSet   The type of the handle set, which must provide get_iterator() and next().
     * @param handle_set   The handle set to sample handles from.
     * @param max_count    Maximum number of handles to consider for random selection (default:
     * MAX_COUNT).
     * @param n            Number of random handles to return (default: 1).
     * @return vector<string>  Vector of randomly selected link handles as strings.
     */
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

/**
 * @brief Maximum count for random link handle selection.
 */
template <typename T>
const size_t AtomDBRunner<T>::MAX_COUNT = 1000;

/**
 * @brief Link query pattern for "contains" relationships.
 */
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

/**
 * @brief Link query pattern for "sentence" relationships.
 */
template <typename T>
const vector<string> AtomDBRunner<T>::sentence_links_query = {
    "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "sentence"};