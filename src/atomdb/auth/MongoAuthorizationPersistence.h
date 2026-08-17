#pragma once

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
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
    void remove(const string& public_key, const string& handle) override;
    void remove_all(const string& public_key) override;

   private:
    mongocxx::pool* pool;
    string database_name;
    string collection_name;

    bsoncxx::document::value make_schema_item(const atomdb_api_types::AccessPermissionEntry& entry);
    string hashed_id(const string& public_key);
    string schema_handle_from_item(bsoncxx::document::view item);
    bool read_full_access(bsoncxx::document::view view);
    void append_schemas_except(bsoncxx::builder::basic::array& schemas,
                               bsoncxx::document::view view,
                               const string& handle_to_skip);
    optional<bsoncxx::document::value> get_document_by_id(const string& id);
};

}  // namespace atomdb
