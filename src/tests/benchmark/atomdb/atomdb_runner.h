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
#include "AtomDBAPITypes.h"
#include "MorkDB.h"
#include "benchmark_runner.h"
#include "benchmark_utils.h"

using namespace std;
using namespace atomdb;

class AtomDBRunner : public Runner {
   public:
    AtomDBRunner(int tid, shared_ptr<AtomDB> db, int iterations);

    /**
     * @brief Selects random link handles from a given handle set.
     *
     * Iterates over handle_set and randomly selects up to n handles (not exceeding max_count).
     *
     * @param handle_set   The handle set to sample handles from.
     * @param max_count    Maximum number of handles to consider for random selection (default:
     * Runner::MAX_COUNT).
     * @param n            Number of random handles to return (default: 1).
     * @return vector<string>  Vector of randomly selected link handles as strings.
     */
    vector<string> get_random_link_handle(shared_ptr<atomdb_api_types::HandleSet> handle_set,
                                          size_t max_count = Runner::MAX_COUNT,
                                          size_t n = 1);

   protected:
    shared_ptr<AtomDB> db_;  // Shared pointer to the AtomDB instance
};