#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <regex>
#include <string>
#include <vector>

#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "benchmark_runner.h"
#include "benchmark_utils.h"

using namespace std;
using namespace atomdb;
using namespace query_engine;
using namespace service_bus;
using namespace atom_space;

class QueryAgentRunner : public Runner {
   public:
    QueryAgentRunner(int tid, shared_ptr<AtomSpace> atom_space, int iterations);

    vector<double> parse_server_side_benchmark_times(const string& log_file);
    bool parse_ms_from_line(const string& line, double& value);

   protected:
    shared_ptr<AtomSpace> atom_space_;  // Shared pointer to the AtomSpace instance

   private:
    static const regex BENCHMARK_PATTERN;
};
