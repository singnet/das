#pragma once

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/options/replace.hpp>
#include <mongocxx/pool.hpp>
#include <string>

#include "AuthorizationPersistence.h"

using namespace std;

namespace atomdb {

/** @brief MongoDB-backed persistence, built by AtomDBFactory for RedisMongoDB backends. */
class MongoAuthorizationPersistence : public AuthorizationPersistence {
   public:
    /**
     * @param pool Mongo pool owned by the backend.
     * @param database_name Mongo database name.
     * @param collection_name access_permissions collection name (hardcoded static on the backend).
     */
    MongoAuthorizationPersistence(mongocxx::pool* pool,
                                  const string& database_name,
                                  const string& collection_name);

    void save(const string& public_key, const AuthorizationEntry& entry) override;
    void remove(const string& public_key, const string& handle) override;
    void remove_all(const string& public_key) override;

   private:
    mongocxx::pool* pool;
    string database_name;
    string collection_name;

    bsoncxx::document::value make_schema_item(const AuthorizationEntry& entry);
};

}  // namespace atomdb
