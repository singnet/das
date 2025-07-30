#include "FitnessFunctionRegistry.h"

#include "FitnessFunction.h"
#include "Utils.h"

// -------------------------------------------------------------------------------------------------
// Add your header here
#include "CountLetterFunction.h"
#include "MultiplyStrengthFunction.h"
#include "UnitTestFunction.h"
// -------------------------------------------------------------------------------------------------

using namespace fitness_functions;
using namespace commons;
bool FitnessFunctionRegistry::INITIALIZED = false;
string FitnessFunctionRegistry::REMOTE_FUNCTION = "remote_fitness_function";
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
        // NOTE: "remote_fitness_function" is reserved and CAN'T be used here.
        // -----------------------------------------------------------------------------------------
        FUNCTION["count_letter"] = make_shared<CountLetterFunction>();
        FUNCTION["multiply_strength"] = make_shared<MultiplyStrengthFunction>();
    }
}

shared_ptr<FitnessFunction> FitnessFunctionRegistry::function(const string& tag) {
    if (INITIALIZED) {
        if (tag == REMOTE_FUNCTION) {
            Utils::error("Invalid use of reserved fitness function tag: " + tag);
        }
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
