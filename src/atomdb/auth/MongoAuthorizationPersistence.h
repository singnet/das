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

/**
 * @brief MongoDB implementation of the authorization persistence interface.
 */
class MongoAuthorizationPersistence : public AuthorizationPersistence {
   public:
    /**
     * @param endpoint MongoDB server endpoint in the format ip:port.
     * @param username MongoDB username.
     * @param password MongoDB password.
     * @param database_name MongoDB database name.
     * @param collection_name MongoDB collection used to store authorization data.
     */
    MongoAuthorizationPersistence(const string& endpoint,
                                  const string& username,
                                  const string& password,
                                  const string& database_name,
                                  const string& collection_name);
    ~MongoAuthorizationPersistence();

    /**
     * @brief Lists all authorization entries for public_key.
     */
    vector<atomdb_api_types::AccessPermissionEntry> list(const string& public_key) override;

    /**
     * @brief Persists an authorization entry for public_key.
     */
    void save(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry) override;

    /**
     * @brief Removes an authorization entry from public_key.
     */
    void remove(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry) override;

    /**
     * @brief Removes all authorization entries for public_key.
     */
    void remove_all(const string& public_key) override;

   private:
    mongocxx::pool* mongodb_pool;
    string database_name;
    string collection_name;

    bsoncxx::document::value entry_to_document(const atomdb_api_types::AccessPermissionEntry& entry);
    shared_ptr<atomdb_api_types::AccessPermissionDocument> get_document(mongocxx::collection& collection,
                                                                        const string& public_key);
};

}  // namespace atomdb
