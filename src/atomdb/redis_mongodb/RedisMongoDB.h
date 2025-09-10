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

enum MONGODB_FIELD { ID = 0, NAME, TARGETS, NAMED_TYPE, size };

class RedisMongoDB : public AtomDB {
   public:
    RedisMongoDB(const string& context = "");
    ~RedisMongoDB();

    static string REDIS_PATTERNS_PREFIX;
    static string REDIS_OUTGOING_PREFIX;
    static string REDIS_INCOMING_PREFIX;
    static uint REDIS_CHUNK_SIZE;
    static string MONGODB_DB_NAME;
    static string MONGODB_NODES_COLLECTION_NAME;
    static string MONGODB_LINKS_COLLECTION_NAME;
    static string MONGODB_PATTERN_INDEX_SCHEMA_COLLECTION_NAME;
    static string MONGODB_FIELD_NAME[MONGODB_FIELD::size];
    static uint MONGODB_CHUNK_SIZE;
    static mongocxx::instance MONGODB_INSTANCE;

    static void initialize_statics(const string& context = "") {
        REDIS_PATTERNS_PREFIX = context + "patterns";
        REDIS_OUTGOING_PREFIX = context + "outgoing_set";
        REDIS_INCOMING_PREFIX = context + "incoming_set";
        REDIS_CHUNK_SIZE = 10000;
        MONGODB_DB_NAME = context + "das";
        MONGODB_NODES_COLLECTION_NAME = context + "nodes";
        MONGODB_LINKS_COLLECTION_NAME = context + "links";
        MONGODB_PATTERN_INDEX_SCHEMA_COLLECTION_NAME = context + "pattern_index_schema";
        MONGODB_FIELD_NAME[MONGODB_FIELD::ID] = "_id";
        MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS] = "targets";
        MONGODB_FIELD_NAME[MONGODB_FIELD::NAME] = "name";
        MONGODB_FIELD_NAME[MONGODB_FIELD::NAMED_TYPE] = "named_type";
        MONGODB_CHUNK_SIZE = 1000;
    }

    // HandleDecoder interface
    shared_ptr<Atom> get_atom(const string& handle);

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema);

    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle);

    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle);

    shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const string& handle);
    shared_ptr<atomdb_api_types::AtomDocument> get_node_document(const string& handle);
    shared_ptr<atomdb_api_types::AtomDocument> get_link_document(const string& handle);

    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(const vector<string>& handles,
                                                                          const vector<string>& fields);
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_node_documents(const vector<string>& handles,
                                                                          const vector<string>& fields);
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_link_documents(const vector<string>& handles,
                                                                          const vector<string>& fields);

    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_matching_atoms(bool is_toplevel, Atom& key);

    bool atom_exists(const string& handle);
    bool node_exists(const string& handle);
    bool link_exists(const string& handle);

    set<string> atoms_exist(const vector<string>& handles);
    set<string> nodes_exist(const vector<string>& handles);
    set<string> links_exist(const vector<string>& handles);

    string add_atom(const atoms::Atom* atom);
    string add_node(const atoms::Node* node);
    string add_link(const atoms::Link* link);

    vector<string> add_atoms(const vector<atoms::Atom*>& atoms);
    vector<string> add_nodes(const vector<atoms::Node*>& nodes);
    vector<string> add_links(const vector<atoms::Link*>& links);

    bool delete_atom(const string& handle, bool delete_link_targets = false);
    bool delete_node(const string& handle, bool delete_link_targets = false);
    bool delete_link(const string& handle, bool delete_link_targets = false);

    uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false);
    uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false);
    uint delete_links(const vector<string>& handles, bool delete_link_targets = false);

    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_filtered_documents(
        const string& collection_name,
        const bsoncxx::builder::stream::document& filter_builder,
        const vector<string>& fields);

    void add_pattern_index_schema(const string& tokens, const vector<vector<string>>& index_entries);
    void re_index_patterns();
    void flush_redis_by_prefix(const string& prefix);

    void drop_all();

    mongocxx::pool* get_mongo_pool() const { return mongodb_pool; }

   private:
    string context;
    bool cluster_flag;
    RedisContextPool* redis_pool;
    mongocxx::pool* mongodb_pool;
    shared_ptr<AtomDBCache> atomdb_cache;
    atomic<uint> patterns_next_score{0};
    atomic<uint> incoming_set_next_score{0};

    map<int, tuple<vector<string>, vector<vector<string>>>> pattern_index_schema_map;
    int pattern_index_schema_next_priority{1};

    shared_ptr<atomdb_api_types::AtomDocument> get_document(const string& handle,
                                                            const string& collection_name);
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_documents(const vector<string>& handles,
                                                                     const vector<string>& fields,
                                                                     const string& collection_name);

    bool document_exists(const string& handle, const string& collection_name);
    set<string> documents_exist(const vector<string>& handles, const string& collection_name);

    bool delete_document(const string& handle,
                         const string& collection_name,
                         bool delete_targets = false);
    uint delete_documents(const vector<string>& handles,
                          const string& collection_name,
                          bool delete_targets = false);

    uint get_next_score(const string& key);
    void set_next_score(const string& key, uint score);
    void reset_scores();

    void add_pattern(const string& handle, const string& pattern_handle);
    void add_patterns(const vector<pair<string, string>>& pattern_handles);
    void delete_pattern(const string& handle);
    void update_pattern(const string& key, const string& value);

    void add_outgoing_set(const string& handle, const vector<string>& outgoing_handles);
    void delete_outgoing_set(const string& handle);

    void add_incoming_set(const string& handle, const string& incoming_handle);
    void delete_incoming_set(const string& handle);
    void update_incoming_set(const string& key, const string& value);

    void load_pattern_index_schema();
    vector<string> match_pattern_index_schema(const Link* link);
    vector<vector<string>> index_entries_combinations(unsigned int arity);

    void redis_setup();
    void mongodb_setup();
    void attention_broker_setup();
};

}  // namespace atomdb
