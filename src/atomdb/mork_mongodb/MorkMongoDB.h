#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <httplib.h>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/pool.hpp>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "MorkMongoDBAPITypes.h"


namespace atomdb {

enum MONGODB_FIELD_ { ID_ = 0, NAME_, TARGETS_, size_ };

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

        static string MONGODB_DB_NAME;
        static string MONGODB_COLLECTION_NAME;
        static string MONGODB_FIELD__NAME[MONGODB_FIELD_::size_];
        static uint MONGODB_CHUNK_SIZE;

        static void initialize_statics() {
            MONGODB_DB_NAME = "das";
            MONGODB_COLLECTION_NAME = "atoms";
            MONGODB_FIELD__NAME[MONGODB_FIELD_::ID_] = "_id";
            MONGODB_FIELD__NAME[MONGODB_FIELD_::TARGETS_] = "targets";
            MONGODB_FIELD__NAME[MONGODB_FIELD_::NAME_] = "name";
            MONGODB_CHUNK_SIZE = 1000;
        }

        shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkTemplateInterface& link_template);
        shared_ptr<atomdb_api_types::HandleList> query_for_targets(shared_ptr<char> link_handle);
        shared_ptr<atomdb_api_types::HandleList> query_for_targets(char* link_handle_ptr);
        shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const char* handle);
        vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(const vector<string>& handles,
                                                                        const vector<string>& fields);
        bool link_exists(const char* link_handle);
        vector<string> links_exist(const vector<string>& link_handles);
        char* add_node(const atomspace::Node* node);
        char* add_link(const atomspace::Link* link);

    private:
        shared_ptr<MorkClient> mork_client;
        mongocxx::client* mongodb_client;
        mongocxx::database mongodb;
        mongocxx::v_noabi::collection mongodb_collection;
        mutex mongodb_mutex;
        mongocxx::pool* mongodb_pool;
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
