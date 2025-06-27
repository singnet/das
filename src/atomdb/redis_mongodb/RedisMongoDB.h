#pragma once

#include <hiredis_cluster/hircluster.h>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <memory>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/pool.hpp>
#include <mutex>
#include <set>
#include <vector>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "RedisContextPool.h"
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
    static uint MONGODB_CHUNK_SIZE;

    static void initialize_statics() {
        REDIS_PATTERNS_PREFIX = "patterns";
        REDIS_TARGETS_PREFIX = "outgoing_set";
        REDIS_CHUNK_SIZE = 10000;
        MONGODB_DB_NAME = "das";
        MONGODB_COLLECTION_NAME = "atoms";
        MONGODB_FIELD_NAME[MONGODB_FIELD::ID] = "_id";
        MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS] = "targets";
        MONGODB_CHUNK_SIZE = 1000;
    }

    // HandleDecoder interface
    shared_ptr<Atom> get_atom(const string& handle);

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(
        const LinkTemplateInterface& link_template);

    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle);

    shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const string& handle);
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(const vector<string>& handles,
                                                                          const vector<string>& fields);

    bool link_exists(const string& link_handle);
    set<string> links_exist(const vector<string>& link_handles);

    string add_node(const atomspace::Node* node);
    vector<string> add_nodes(const vector<atomspace::Node*>& nodes);

    string add_link(const atomspace::Link* link);
    vector<string> add_links(const vector<atomspace::Link*>& links);

    bool delete_atom(const string& handle);
    uint delete_atoms(const vector<string>& handles);

   private:
    bool cluster_flag;
    RedisContextPool* redis_pool;
    mongocxx::pool* mongodb_pool;
    shared_ptr<AtomDBCache> atomdb_cache;

    void redis_setup();
    void mongodb_setup();
    void attention_broker_setup();
};

}  // namespace atomdb
