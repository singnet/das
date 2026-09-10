#pragma once

#include <memory>
#include <mongocxx/collection.hpp>
#include <mongocxx/pool.hpp>
#include <string>
#include <vector>

#include "AuthorizationPersistence.h"
#include "RedisMongoDBAPITypes.h"

using namespace std;

namespace atomdb {

/**
 * @brief MongoDB-backed implementation of AuthorizationPersistence.
 *
 * Stores one authorization document per public key in a MongoDB collection.
 * Documents contain a full_access flag and an allowed_schemas array.
 */
class MongodbAuthorizationPersistence : public AuthorizationPersistence {
   public:
    /**
     * @brief Connects to MongoDB and verifies the target database is reachable.
     *
     * @param endpoint MongoDB server endpoint in ip:port format.
     * @param username MongoDB username.
     * @param password MongoDB password.
     * @param database_name Database that stores authorization documents.
     * @param collection_name Collection that stores authorization documents.
     * @throws std::runtime_error if configuration is invalid or the connection fails.
     */
    MongodbAuthorizationPersistence(const string& endpoint,
                                    const string& username,
                                    const string& password,
                                    const string& database_name,
                                    const string& collection_name);
    ~MongodbAuthorizationPersistence();

    /**
     * @brief Persists schema-based grants for a public key.
     *
     * Merges the provided schemas into the existing document when one already
     * exists. Raises an error if the key already has unrestricted access.
     *
     * @param public_key Public key receiving the grant.
     * @param schemas Schema rules and permission bitmasks to persist.
     * @throws std::runtime_error if the key already has unrestricted access or the MongoDB update fails.
     */
    void grant(const string& public_key, vector<pair<LinkSchema, unsigned int>>& schemas) override;

    /**
     * @brief Persists unrestricted access for a public key.
     *
     * Replaces any existing document for the key with full_access=true and an
     * empty allowed_schemas array.
     *
     * @param public_key Public key receiving unrestricted access.
     * @throws std::runtime_error if the MongoDB update fails.
     */
    void grant_unrestricted(const string& public_key) override;

    /**
     * @brief Removes the authorization document for a public key.
     *
     * @param public_key Public key whose grants should be removed.
     * @throws std::runtime_error if MongoDB does not acknowledge the delete.
     */
    void revoke(const string& public_key) override;

   private:
    mongocxx::pool* mongodb_pool;
    string database_name;
    string collection_name;

    static bsoncxx::document::value to_bson(const string& public_key,
                                            vector<pair<LinkSchema, unsigned int>>& schemas);
    static bsoncxx::document::value to_bson(
        const atomdb_api_types::MongodbAccessPermissionDocument& document,
        vector<pair<LinkSchema, unsigned int>>& schemas);

    static void append_schema_entries(bsoncxx::builder::basic::array& allowed_schemas,
                                      vector<pair<LinkSchema, unsigned int>>& schemas);
    shared_ptr<atomdb_api_types::MongodbAccessPermissionDocument> get_document(
        mongocxx::collection& collection, const string& public_key);
};

}  // namespace atomdb
