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
using namespace atomspace;

class QueryAgentRunner : public Runner {
   public:
    QueryAgentRunner(int tid, shared_ptr<AtomSpace> atom_space, int iterations);

   protected:
    shared_ptr<AtomSpace> atom_space_;  // Shared pointer to the AtomSpace instance
};