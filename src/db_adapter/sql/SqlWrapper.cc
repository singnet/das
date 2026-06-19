#include "SqlWrapper.h"

#include <utf8.h>

using namespace db_adapter;

size_t SQLWrapper::MAX_VALUE_SIZE = 1000;

SQLWrapper::SQLWrapper(shared_ptr<DatabaseConnection> db_conn, shared_ptr<DatabaseMapper> mapper)
    : DatabaseWrapper(db_conn, mapper) {}

bool SQLWrapper::sanitize_value(string& value) {
    value = Utils::trim(value);

    if (value.empty() || value.size() > MAX_VALUE_SIZE || !utf8::is_valid(value)) return false;

    Utils::replace_all(value, "\n", " ");

    Utils::replace_all(value, "\t", " ");

    return true;
}
