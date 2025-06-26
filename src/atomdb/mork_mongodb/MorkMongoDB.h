#pragma once

#include <httplib.h>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <memory>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/pool.hpp>
#include <mutex>
#include <vector>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "MorkMongoDBAPITypes.h"
// #include "RedisMongoDB.h"

namespace atomdb {

enum MONGODB_FIELD2 { ID2 = 0, NAME, TARGETS2, size2 };

class MorkClient {
   public:
    MorkClient(const string& base_url = "");
    ~MorkClient();
    string post(const string& data, const string& pattern = "$x", const string& template_ = "$x");
    vector<string> get(const string& pattern, const string& template_);

   private:
    string base_url_;
    httplib::Client cli;
    httplib::Result send_request(const string& method, const string& path, const string& data = "");
    string url_encode(const string& value);
};

class MorkMongoDB : public AtomDB {
   public:
    MorkMongoDB();
    ~MorkMongoDB();

    static string MONGODB_DB_NAME2;
    static string MONGODB_COLLECTION_NAME2;
    static string MONGODB_FIELD_NAME2[MONGODB_FIELD2::size2];
    static uint MONGODB_CHUNK_SIZE2;

    static void initialize_statics() {
        MONGODB_DB_NAME2 = "das";
        MONGODB_COLLECTION_NAME2 = "atoms";
        MONGODB_FIELD_NAME2[MONGODB_FIELD2::ID2] = "_id";
        MONGODB_FIELD_NAME2[MONGODB_FIELD2::TARGETS2] = "targets";
        MONGODB_FIELD_NAME2[MONGODB_FIELD2::NAME] = "name";
        MONGODB_CHUNK_SIZE2 = 1000;
    }

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(
        const LinkTemplateInterface& link_template);
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(shared_ptr<char> link_handle);
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(char* link_handle_ptr);
    shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const char* handle);
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(const vector<string>& handles,
                                                                          const vector<string>& fields);
    bool link_exists(const char* link_handle);
    set<string> links_exist(const vector<string>& link_handles);
    char* add_node(const atomspace::Node* node);
    char* add_link(const atomspace::Link* link);

   private:
    shared_ptr<MorkClient> mork_client;
    mongocxx::pool* mongodb_pool;
    // shared_ptr<RedisMongoDB> redis_mongodb;
    shared_ptr<AtomDBCache> atomdb_cache;

    void mork_setup();
    void mongodb_setup();
    void attention_broker_setup();

    struct Node {
        string name;
        vector<Node> targets;
    };
    vector<string> tokenize_expression(const string& expr);
    Node parse_tokens_to_node(const vector<string>& tokens, size_t& pos);
    Node parse_expression_tree(const string& expr);
    string resolve_node_handle(Node& node);
};

}  // namespace atomdb
