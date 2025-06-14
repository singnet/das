#pragma once

#include <hiredis_cluster/hircluster.h>

#include <bsoncxx/json.hpp>
#include <memory>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mutex>
#include <vector>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "RedisMongoDBAPITypes.h"

using namespace std;

namespace atomdb {

enum MONGODB_FIELD { ID = 0, TARGETS, size };

class RedisMongoDB : public AtomDB {
   public:
    RedisMongoDB();
    ~RedisMongoDB();

    static string REDIS_PATTERNS_PREFIX;
    static string REDIS_TARGETS_PREFIX;
    static uint REDIS_CHUNK_SIZE;
    static string MONGODB_DB_NAME;
    static string MONGODB_COLLECTION_NAME;
    static string MONGODB_FIELD_NAME[MONGODB_FIELD::size];

    static void initialize_statics() {
        REDIS_PATTERNS_PREFIX = "patterns";
        REDIS_TARGETS_PREFIX = "outgoing_set";
        REDIS_CHUNK_SIZE = 10000;
        MONGODB_DB_NAME = "das";
        MONGODB_COLLECTION_NAME = "atoms";
        MONGODB_FIELD_NAME[MONGODB_FIELD::ID] = "_id";
        MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS] = "targets";
    }

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(shared_ptr<char> pattern_handle);
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(shared_ptr<char> link_handle);
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(char* link_handle_ptr);
    shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const char* handle);
    bool link_exists(const char* link_handle);
    std::vector<std::string> links_exist(const std::vector<std::string>& link_handles);
    char* add_node(const char* type,
                   const char* name,
                   const atomdb_api_types::CustomAttributesMap& custom_attributes = {});
    char* add_link(const char* type,
                   char** targets,
                   size_t targets_size,
                   const atomdb_api_types::CustomAttributesMap& custom_attributes = {});

   private:
    bool cluster_flag;
    redisClusterContext* redis_cluster;
    redisContext* redis_single;
    mongocxx::client* mongodb_client;
    mongocxx::database mongodb;
    mongocxx::v_noabi::collection mongodb_collection;
    mutex mongodb_mutex;
    mongocxx::pool* mongodb_pool;
    shared_ptr<AtomDBCache> atomdb_cache;

    mongocxx::database get_database();

    void redis_setup();
    void mongodb_setup();
    void attention_broker_setup();
};

}  // namespace atomdb
