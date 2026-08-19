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
     * @param endpoint ip:port
     * @param database_name Mongo database name
     * @param collection_name access_permissions collection name
     */
    MongoAuthorizationPersistence(const string& endpoint,
                                  const string& username,
                                  const string& password,
                                  const string& database_name,
                                  const string& collection_name);
    ~MongoAuthorizationPersistence();

    void save(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry) override;
    void remove(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry) override;
    void remove_all(const string& public_key) override;

   private:
    mongocxx::pool* mongodb_pool;
    string database_name;
    string collection_name;

    bsoncxx::document::value entry_to_document(const atomdb_api_types::AccessPermissionEntry& entry);
    shared_ptr<atomdb_api_types::AccessPermissionDocument> get_document(mongocxx::collection& collection,
                                                                        const string& public_key);
    mongocxx::collection get_mongodb_collection();
};

}  // namespace atomdb
