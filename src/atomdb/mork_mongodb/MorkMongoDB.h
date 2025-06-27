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
#include "AtomSpaceTypes.h"
#include "RedisMongoDB.h"

namespace atomdb {

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

    static void initialize_statics() { RedisMongoDB::initialize_statics(); }

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(
        const LinkTemplateInterface& link_template);
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(shared_ptr<char> link_handle);
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(char* link_handle_ptr);

    // Delegates for RedisMongoDB
    shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const char* handle) {
        return this->redis_mongodb->get_atom_document(handle);
    }
    bool link_exists(const char* link_handle) { return this->redis_mongodb->link_exists(link_handle); }
    set<string> links_exist(const vector<string>& link_handles) {
        return this->redis_mongodb->links_exist(link_handles);
    }
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(const vector<string>& handles,
                                                                          const vector<string>& fields) {
        return this->redis_mongodb->get_atom_documents(handles, fields);
    }
    char* add_node(const atomspace::Node* node) { return this->redis_mongodb->add_node(node); }
    char* add_link(const atomspace::Link* link) { return this->redis_mongodb->add_link(link); }

   private:
    shared_ptr<RedisMongoDB> redis_mongodb;
    mongocxx::pool* mongodb_pool;
    shared_ptr<AtomDBCache> atomdb_cache;
    shared_ptr<MorkClient> mork_client;

    void mork_setup();
    void attention_broker_setup();

    vector<string> tokenize_expression(const string& expr);
    const atomspace::Atom* parse_tokens_to_atom(const vector<string>& tokens, size_t& pos);
};

}  // namespace atomdb
