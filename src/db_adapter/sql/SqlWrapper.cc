#include "SqlWrapper.h"

using namespace db_adapter;

SQLWrapper::SQLWrapper(DatabaseConnection& db_client, shared_ptr<DatabaseMapper> mapper)
    : DatabaseWrapper(db_client, mapper) {}