#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <httplib.h>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "RedisMongoDBAPITypes.h"


namespace atomdb {

enum MONGODB_FIELD { ID = 0, NAME, TARGETS, size };

class MorkClient {
    public:
        MorkClient(const std::string& base_url = "");
        ~MorkClient();
        std::string post(const std::string& data, const std::string& pattern = "$x", const std::string& template_ = "$x");
        vector<std::string> get(const std::string& pattern, const std::string& template_);

    private:
        httplib::Client cli;
        std::string base_url_;
        httplib::Result sendRequest(const std::string& method, const std::string& path, const std::string& data);
        std::string urlEncode(const std::string& value);
};

class MorkMongoDB : public AtomDB {
    public:
        MorkMongoDB();
        ~MorkMongoDB();

        static std::string MONGODB_DB_NAME;
        static std::string MONGODB_COLLECTION_NAME;
        static std::string MONGODB_FIELD_NAME[MONGODB_FIELD::size];

        static void initialize_statics() {
            MONGODB_DB_NAME = "das";
            MONGODB_COLLECTION_NAME = "atoms";
            MONGODB_FIELD_NAME[MONGODB_FIELD::ID] = "_id";
            MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS] = "targets";
            MONGODB_FIELD_NAME[MONGODB_FIELD::NAME] = "name";
        }

        std::shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(std::shared_ptr<char> pattern_handle);
        std::shared_ptr<atomdb_api_types::HandleList> query_for_targets(std::shared_ptr<char> link_handle);
        std::shared_ptr<atomdb_api_types::HandleList> query_for_targets(char* link_handle_ptr);
        std::shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const char* handle);
        bool link_exists(const char* link_handle);
        std::vector<std::string> links_exist(const std::vector<std::string>& link_handles);
        char* add_node(
            const char* type,
            const char* name,
            const atomdb_api_types::CustomAttributesMap& custom_attributes = {}
        );
        char* add_link(
            const char* type,
            char** targets,
            size_t targets_size,
            const atomdb_api_types::CustomAttributesMap& custom_attributes = {}
        );

    private:
        std::shared_ptr<MorkClient> mork_client;
        mongocxx::client* mongodb_client;
        mongocxx::database mongodb;
        mongocxx::v_noabi::collection mongodb_collection;
        std::mutex mongodb_mutex;
        mongocxx::pool* mongodb_pool;
        std::shared_ptr<AtomDBCache> atomdb_cache;

        mongocxx::database get_database();
        void mork_setup();
        void mongodb_setup();
        void attention_broker_setup();

        struct Node {
            std::string name;
            std::vector<Node> targets;
        };
        std::vector<std::string> tokenize(const std::string& expr);
        Node parse_tokens(const std::vector<std::string>& tokens, size_t& pos);
        Node parse_expression(const std::string& expr);
        std::string get_handle(Node& atom);
};

}  // namespace atomdb
