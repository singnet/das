#ifndef _QUERY_ENGINE_ATOMDBAPITYPES_H
#define _QUERY_ENGINE_ATOMDBAPITYPES_H

#include <hiredis_cluster/hircluster.h>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include "Utils.h"

using namespace std;
using namespace commons;

namespace query_engine {
namespace atomdb_api_types {


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


class HandleList {

public:

    HandleList() {}
    virtual ~HandleList() {}

    virtual const char *get_handle(unsigned int index) = 0;
    virtual unsigned int size() = 0;
};

class RedisSet : public HandleList {

public:

    RedisSet(redisReply *reply);
    ~RedisSet();

    const char *get_handle(unsigned int index);
    unsigned int size();

private:

    unsigned int handles_size;
    char **handles;
    redisReply *redis_reply;
};

class RedisStringBundle : public HandleList {

public:

    RedisStringBundle(redisReply *reply);
    ~RedisStringBundle();

    const char *get_handle(unsigned int index);
    unsigned int size();

private:

    unsigned int handles_size;
    char **handles;
    redisReply *redis_reply;
};

class AtomDocument {

public:

    AtomDocument() {}
    virtual ~AtomDocument() {}

    virtual const char *get(const string &key) = 0;
    virtual const char *get(const string &array_key, unsigned int index) = 0;
    virtual unsigned int get_size(const string &array_key) = 0;
};

class MongodbDocument : public AtomDocument {

public:

    MongodbDocument(core::v1::optional<bsoncxx::v_noabi::document::value>& document);
    ~MongodbDocument();

    const char *get(const string &key);
    virtual const char *get(const string &array_key, unsigned int index);
    virtual unsigned int get_size(const string &array_key);

private:

    core::v1::optional<bsoncxx::v_noabi::document::value> document;
};

} // namespace atomdb_api_types
} // namespace query_engine

#endif // _QUERY_ENGINE_ATOMDBAPITYPES_H
