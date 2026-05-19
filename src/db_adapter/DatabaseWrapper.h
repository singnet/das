#pragma once

#include <memory>

#include "DatabaseConnection.h"
#include "DatabaseMapper.h"
#include "DatabaseTypes.h"

using namespace std;

namespace db_adapter {

/**
 * @class DatabaseWrapper
 * @brief Generic interface for a database connection wrapper.
 */
class DatabaseWrapper {
   public:
    DatabaseWrapper(DatabaseConnection& db_client, shared_ptr<DatabaseMapper> mapper);
    virtual ~DatabaseWrapper() = default;

    unsigned int mapper_handle_trie_size();

   protected:
    DatabaseConnection& db_client;
    shared_ptr<DatabaseMapper> mapper;
};

}  // namespace db_adapter