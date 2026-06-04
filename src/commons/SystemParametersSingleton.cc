#include "SystemParametersSingleton.h"

#include "JsonConfigParser.h"

using namespace commons;
using namespace std;

bool SystemParametersSingleton::INITIALIZED = false;
shared_ptr<SystemParameters> SystemParametersSingleton::PARAMS = shared_ptr<SystemParameters>(nullptr);
mutex SystemParametersSingleton::API_MUTEX;

// -------------------------------------------------------------------------------------------------
// Public methods

void SystemParametersSingleton::init(const JsonConfig& das_config) {
    lock_guard<mutex> semaphore(API_MUTEX);
    PARAMS = make_shared<SystemParameters>(SystemParameters(das_config));
    INITIALIZED = true;
}

void SystemParametersSingleton::init_from_file(const string& path) {
    init(JsonConfigParser::load(path));
}

shared_ptr<SystemParameters> SystemParametersSingleton::get_instance() {
    lock_guard<mutex> semaphore(API_MUTEX);
    if (!INITIALIZED || PARAMS == nullptr) {
        RAISE_ERROR("SystemParametersSingleton not initialized");
    }
    return PARAMS;
}

void SystemParametersSingleton::provide(shared_ptr<SystemParameters> parameters) {
    if (parameters == nullptr) {
        RAISE_ERROR("SystemParametersSingleton::provide(): parameters cannot be nullptr");
    }
    lock_guard<mutex> semaphore(API_MUTEX);
    PARAMS = parameters;
    INITIALIZED = true;
}
