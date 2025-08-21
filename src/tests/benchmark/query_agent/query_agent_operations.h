#pragma once

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "query_agent_runner.h"

extern const size_t BATCH_SIZE;

class PatternMatchingQuery : public QueryAgentRunner {
   public:
    using QueryAgentRunner::QueryAgentRunner;

    void simple_query(string log_file);
    void complex_query(string log_file);
    void positive_importance();
};