#pragma once

#include <hiredis_cluster/hircluster.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <vector>

#include "Atom.h"
#include "AtomDBAPITypes.h"
#include "HandleDecoder.h"
#include "Node.h"
#include "Properties.h"
#include "Utils.h"

using namespace std;
using namespace commons;
using namespace atoms;

namespace atomdb {
namespace atomdb_api_types {

class HandleSetRedis : public HandleSet {
    friend class HandleSetRedisIterator;

   public:
    HandleSetRedis(bool delete_replies_on_destruction = true);
    HandleSetRedis(redisReply* reply, bool delete_replies_on_destruction = true);
    ~HandleSetRedis();

    unsigned int size();
    void append(shared_ptr<HandleSet> other);
    shared_ptr<HandleSetIterator> get_iterator();

   private:
    unsigned int handles_size;
    vector<redisReply*> replies;
    bool delete_replies_on_destruction;
};

class HandleSetRedisIterator : public HandleSetIterator {
   public:
    HandleSetRedisIterator(HandleSetRedis* handle_set);
    ~HandleSetRedisIterator();

    char* next();

   private:
    HandleSetRedis* handle_set;
    unsigned int outer_idx;
    unsigned int inner_idx;
};

class RedisStringBundle : public HandleList {
   public:
    RedisStringBundle(redisReply* reply);
    ~RedisStringBundle();

    const char* get_handle(unsigned int index);
    unsigned int size();

   private:
    unsigned int handles_size;
    char** handles;
    redisReply* redis_reply;
};

class MongodbDocument : public AtomDocument {
   public:
    MongodbDocument(bsoncxx::v_noabi::stdx::optional<bsoncxx::v_noabi::document::value>& document);
    MongodbDocument(const atoms::Node* node);
    MongodbDocument(const atoms::Link* link, HandleDecoder& db);
    ~MongodbDocument();

    const char* get(const string& key);
    virtual const char* get(const string& array_key, unsigned int index);
    virtual unsigned int get_size(const string& array_key);
    virtual bool contains(const string& key);

    bsoncxx::v_noabi::document::value value();
    bsoncxx::v_noabi::document::view get_object(const string& key);
    Properties extract_custom_attributes(const bsoncxx::v_noabi::document::view& doc);

   private:
    bsoncxx::v_noabi::stdx::optional<bsoncxx::v_noabi::document::value> document;
};

}  // namespace atomdb_api_types
}  // namespace atomdb
