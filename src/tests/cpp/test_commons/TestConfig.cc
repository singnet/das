#include "TestConfig.h"

string TestConfig::DAS_REDIS_HOSTNAME = "localhost";
string TestConfig::DAS_REDIS_PORT = "29000";
string TestConfig::DAS_USE_REDIS_CLUSTER = "false";
string TestConfig::DAS_MONGODB_HOSTNAME = "localhost";
string TestConfig::DAS_MONGODB_PORT = "28000";
string TestConfig::DAS_MONGODB_USERNAME = "dbadmin";
string TestConfig::DAS_MONGODB_PASSWORD = "dassecret";
string TestConfig::DAS_DISABLE_ATOMDB_CACHE = "false";

void TestConfig::load_environment(bool replace_existing) {
    setenv("DAS_REDIS_HOSTNAME", DAS_REDIS_HOSTNAME.c_str(), replace_existing);
    setenv("DAS_REDIS_PORT", DAS_REDIS_PORT.c_str(), replace_existing);
    setenv("DAS_USE_REDIS_CLUSTER", DAS_USE_REDIS_CLUSTER.c_str(), replace_existing);
    setenv("DAS_MONGODB_HOSTNAME", DAS_MONGODB_HOSTNAME.c_str(), replace_existing);
    setenv("DAS_MONGODB_PORT", DAS_MONGODB_PORT.c_str(), replace_existing);
    setenv("DAS_MONGODB_USERNAME", DAS_MONGODB_USERNAME.c_str(), replace_existing);
    setenv("DAS_MONGODB_PASSWORD", DAS_MONGODB_PASSWORD.c_str(), replace_existing);
    setenv("DAS_DISABLE_ATOMDB_CACHE", DAS_DISABLE_ATOMDB_CACHE.c_str(), true);
}

void TestConfig::disable_atomdb_cache() { setenv("DAS_DISABLE_ATOMDB_CACHE", "true", true); }