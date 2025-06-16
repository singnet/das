#include "FitnessFunctionRegistry.h"
#include "FitnessFunction.h"
#include "Utils.h"

using namespace fitness_functions;
using namespace commons;
bool FitnessFunctionRegistry::INITIALIZED = false;
map<string, shared_ptr<FitnessFunction>> FitnessFunctionRegistry::FUNCTION;

// Add your header here
#include "UnitTestFunction.h"

void FitnessFunctionRegistry::initialize_statics() {
    if (INITIALIZED) {
        Utils::error(
            "FitnessFunctionRegistry already initialized. "
            "FitnessFunctionRegistry::init() should be called only once.");
    } else {
        INITIALIZED = true;
        // Add your function here using a unique string key
        FUNCTION["unit_test"] = make_shared<UnitTestFunction>();
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
        Utils::error("FitnessFunctionRegistry isn't initialized. Call FitnessFunctionRegistry::init() first.");
    }
    return shared_ptr<FitnessFunction>(nullptr);
}

//FitnessFunctionRegistry::function.insert(std::make_pair("unit_test", new UnitTestFunction()));

/*
map<string, shared_ptr<FitnessFunction>> FitnessFunctionRegistry::function = {
    {"unit_test", static_cast<FitnessFunction>(make_shared<UnitTestFunction>(new UnitTestFunction()))},
};
*/
