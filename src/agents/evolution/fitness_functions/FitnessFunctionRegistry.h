#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "FitnessFunction.h"

using namespace std;

namespace fitness_functions {

/**
 * Registry for fitness functions used by the Query Evolution agent.
 *
 * In order to register a fitness function, edit FitnessFunctionRegistry.cc
 */
class FitnessFunctionRegistry {
   public:
    ~FitnessFunctionRegistry() {}
    static shared_ptr<FitnessFunction> function(const string& tag);
    static void initialize_statics();

   private:
    FitnessFunctionRegistry() {}

    static bool INITIALIZED;
    static map<string, shared_ptr<FitnessFunction>> FUNCTION;
};

}  // namespace fitness_functions
