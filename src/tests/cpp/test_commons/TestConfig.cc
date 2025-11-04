#include "TestConfig.h"

string TestConfig::DAS_REDIS_HOSTNAME = "localhost";
string TestConfig::DAS_REDIS_PORT = "40020";
string TestConfig::DAS_USE_REDIS_CLUSTER = "false";
string TestConfig::DAS_MONGODB_HOSTNAME = "localhost";
string TestConfig::DAS_MONGODB_PORT = "40021";
string TestConfig::DAS_MONGODB_USERNAME = "admin";
string TestConfig::DAS_MONGODB_PASSWORD = "admin";
string TestConfig::DAS_DISABLE_ATOMDB_CACHE = "false";
string TestConfig::DAS_MORK_HOSTNAME = "localhost";
string TestConfig::DAS_MORK_PORT = "8000";

void TestConfig::load_environment(bool replace_existing) {
    setenv("DAS_REDIS_HOSTNAME", DAS_REDIS_HOSTNAME.c_str(), replace_existing);
    setenv("DAS_REDIS_PORT", DAS_REDIS_PORT.c_str(), replace_existing);
    setenv("DAS_USE_REDIS_CLUSTER", DAS_USE_REDIS_CLUSTER.c_str(), replace_existing);
    setenv("DAS_MONGODB_HOSTNAME", DAS_MONGODB_HOSTNAME.c_str(), replace_existing);
    setenv("DAS_MONGODB_PORT", DAS_MONGODB_PORT.c_str(), replace_existing);
    setenv("DAS_MONGODB_USERNAME", DAS_MONGODB_USERNAME.c_str(), replace_existing);
    setenv("DAS_MONGODB_PASSWORD", DAS_MONGODB_PASSWORD.c_str(), replace_existing);
    setenv("DAS_DISABLE_ATOMDB_CACHE", DAS_DISABLE_ATOMDB_CACHE.c_str(), true);
    setenv("DAS_MORK_HOSTNAME", DAS_MORK_HOSTNAME.c_str(), replace_existing);
    setenv("DAS_MORK_PORT", DAS_MORK_PORT.c_str(), replace_existing);
}

void TestConfig::set_atomdb_cache(bool enable) { setenv("DAS_DISABLE_ATOMDB_CACHE", enable ? "false" : "true", true); }
