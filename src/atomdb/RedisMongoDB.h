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
#include "AtomDBAPITypes.h"

using namespace std;

namespace atomdb {

enum MONGODB_FIELD { ID = 0, size };

// -------------------------------------------------------------------------------------------------
// NOTE TO REVIEWER:
//
// This class will be replaced/integrated by/with classes already implemented in das-atom-db.
//
// However, that classes will need to be revisited in order to allow the methods implemented here
// because although the design of such methods is nasty, they have the string advantage of
// allowing the reuse of structures allocated by the DBMS (Redis an MongoDB) withpout the need
// of re-allocation of other dataclasses. Although this nasty behavior may not be desirable
// outside the DAS bounds, it's quite appealing inside the query engine (and perhaps other
// parts of internal stuff).
//
// I think it's pointless to make any further documentation while we don't make this integrfation.
// -------------------------------------------------------------------------------------------------

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
    }

    vector<string> query_for_pattern(shared_ptr<char> pattern_handle);
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(shared_ptr<char> link_handle);
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(char* link_handle_ptr);
    shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const char* handle);

   private:
    bool cluster_flag;
    redisClusterContext* redis_cluster;
    redisContext* redis_single;
    mongocxx::client* mongodb_client;
    mongocxx::database mongodb;
    mongocxx::v_noabi::collection mongodb_collection;
    mutex mongodb_mutex;
    mongocxx::pool* mongodb_pool;

    mongocxx::database get_database();

    void redis_setup();
    void mongodb_setup();
    void attention_broker_setup();
};

}  // namespace atomdb
