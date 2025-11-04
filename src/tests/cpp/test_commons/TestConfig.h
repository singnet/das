#pragma once

#include <string>

using namespace std;

class TestConfig {
   public:
    static string DAS_REDIS_HOSTNAME;
    static string DAS_REDIS_PORT;
    static string DAS_USE_REDIS_CLUSTER;
    static string DAS_MONGODB_HOSTNAME;
    static string DAS_MONGODB_PORT;
    static string DAS_MONGODB_USERNAME;
    static string DAS_MONGODB_PASSWORD;
    static string DAS_MORK_HOSTNAME;
    static string DAS_MORK_PORT;
    static string DAS_DISABLE_ATOMDB_CACHE;

    static void load_environment(bool replace_existing = true);
    static void set_atomdb_cache(bool enable);
};
