#pragma once

#include <hiredis_cluster/hircluster.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <vector>

#include "AtomDBAPITypes.h"
#include "Utils.h"

using namespace std;
using namespace commons;

namespace atomdb {
namespace atomdb_api_types {

class HandleSetRedis : public HandleSet {
    friend class HandleSetRedisIterator;

   public:
    HandleSetRedis();
    HandleSetRedis(redisReply* reply);
    ~HandleSetRedis();

    unsigned int size();
    void append(shared_ptr<HandleSet> other);
    shared_ptr<HandleSetIterator> get_iterator();

   private:
    unsigned int handles_size;
    vector<redisReply*> replies;
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
    MongodbDocument(core::v1::optional<bsoncxx::v_noabi::document::value>& document);
    ~MongodbDocument();

    const char* get(const string& key);
    virtual const char* get(const string& array_key, unsigned int index);
    virtual unsigned int get_size(const string& array_key);

   private:
    core::v1::optional<bsoncxx::v_noabi::document::value> document;
};

}  // namespace atomdb_api_types
}  // namespace atomdb
