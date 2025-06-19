#include "FitnessFunctionRegistry.h"

#include "FitnessFunction.h"
#include "Utils.h"

// -------------------------------------------------------------------------------------------------
// Add your header here
#include "UnitTestFunction.h"
// -------------------------------------------------------------------------------------------------

using namespace fitness_functions;
using namespace commons;
bool FitnessFunctionRegistry::INITIALIZED = false;
map<string, shared_ptr<FitnessFunction>> FitnessFunctionRegistry::FUNCTION;

void FitnessFunctionRegistry::initialize_statics() {
    if (INITIALIZED) {
        Utils::error(
            "FitnessFunctionRegistry already initialized. "
            "FitnessFunctionRegistry::init() should be called only once.");
    } else {
        INITIALIZED = true;
        FUNCTION["unit_test"] = make_shared<UnitTestFunction>();
        // -----------------------------------------------------------------------------------------
        // Add your function here using a unique string key
        // FUNCTION["my_function_tag"] = make_shared<MyFunction>();
        // -----------------------------------------------------------------------------------------
    }
}

shared_ptr<FitnessFunction> FitnessFunctionRegistry::function(const string& tag) {
    if (INITIALIZED) {
        if (FUNCTION.find(tag) != FUNCTION.end()) {
            return FUNCTION[tag];
        } else {
            Utils::error("Unkown fitness function: " + tag);
        }
    } else {
        Utils::error(
            "FitnessFunctionRegistry isn't initialized. Call "
            "FitnessFunctionRegistry::initialize_statics() first.");
    }
    return shared_ptr<FitnessFunction>(nullptr);
}
