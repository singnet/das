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
#include "MorkDBAPITypes.h"
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

class MorkDB : public RedisMongoDB {
   public:
    MorkDB();
    ~MorkDB();

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(
        const LinkTemplateInterface& link_template) override;

   private:
    shared_ptr<AtomDBCache> atomdb_cache;
    shared_ptr<MorkClient> mork_client;

    void mork_setup();
    vector<string> tokenize_expression(const string& expr);
    const atoms::Atom* parse_tokens_to_atom(const vector<string>& tokens, size_t& pos);
};

}  // namespace atomdb
