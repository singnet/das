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
    void save(const string& public_key, const AuthorizationSchema& entry) override;
    void remove(const string& public_key, const AuthorizationSchema& entry) override;
    void remove_all(const string& public_key) override;

   private:
    mongocxx::pool* mongodb_pool;
    string database_name;
    string collection_name;

    static vector<AuthorizationSchema> schemas_from_document(
        const atomdb_api_types::AccessPermissionDocument& document);
    shared_ptr<atomdb_api_types::MongodbAccessPermissionDocument> get_document(
        mongocxx::collection& collection, const string& public_key);
};

}  // namespace atomdb
