#include "SqlWrapper.h"

using namespace db_adapter;

SQLWrapper::SQLWrapper(shared_ptr<DatabaseConnection> db_conn, shared_ptr<DatabaseMapper> mapper)
    : DatabaseWrapper(db_conn, mapper) {}