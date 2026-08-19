#pragma once

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <memory>
#include <mongocxx/collection.hpp>
#include <mongocxx/options/replace.hpp>
#include <mongocxx/pool.hpp>
#include <string>

#include "AuthorizationPersistence.h"

using namespace std;

namespace atomdb {

class MongoAuthorizationPersistence : public AuthorizationPersistence {
   public:
    /**
     * @param pool Mongo pool
     * @param database_name Mongo database name
     * @param collection_name access_permissions collection name
     */
    MongoAuthorizationPersistence(mongocxx::pool* pool,
                                  const string& database_name,
                                  const string& collection_name);

    void save(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry) override;
    void remove(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry) override;
    void remove_all(const string& public_key) override;

   private:
    mongocxx::pool* pool;
    string database_name;
    string collection_name;

    bsoncxx::document::value entry_to_document(const atomdb_api_types::AccessPermissionEntry& entry);
    shared_ptr<atomdb_api_types::AccessPermissionDocument> get_document(mongocxx::collection& collection,
                                                                        const string& public_key);
};

}  // namespace atomdb
