#pragma once

#include <memory>
#include <mongocxx/collection.hpp>
#include <mongocxx/pool.hpp>
#include <string>

#include "AuthorizationPersistence.h"
#include "RedisMongoDBAPITypes.h"

using namespace std;

namespace atomdb {

/**
 * @brief MongoDB implementation of the authorization persistence interface.
 */
class MongodbAuthorizationPersistence : public AuthorizationPersistence {
   public:
    /**
     * @param endpoint MongoDB server endpoint in the format ip:port.
     * @param username MongoDB username.
     * @param password MongoDB password.
     * @param database_name MongoDB database name.
     * @param collection_name MongoDB collection used to store authorization data.
     */
    MongodbAuthorizationPersistence(const string& endpoint,
                                    const string& username,
                                    const string& password,
                                    const string& database_name,
                                    const string& collection_name);
    ~MongodbAuthorizationPersistence();

    vector<AuthorizationSchema> list(const string& public_key) override;
    void save(const string& public_key, const AuthorizationSchema& schema) override;
    void remove(const string& public_key, const AuthorizationSchema& schema) override;
    void remove_all(const string& public_key) override;

   private:
    mongocxx::pool* mongodb_pool;
    string database_name;
    string collection_name;

    static bsoncxx::document::value to_bson(const AuthorizationProfile& profile);
    shared_ptr<atomdb_api_types::MongodbAccessPermissionDocument> get_document(
        mongocxx::collection& collection, const string& public_key);
    void delete_document(mongocxx::collection& collection, const string& public_key);
};

}  // namespace atomdb
