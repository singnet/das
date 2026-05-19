#include "DatabaseWrapper.h"

#include "DatabaseMapper.h"

using namespace std;
using namespace db_adapter;

DatabaseWrapper::DatabaseWrapper(DatabaseConnection& db_client, shared_ptr<DatabaseMapper> mapper)
    : db_client(db_client), mapper(mapper) {}

unsigned int DatabaseWrapper::mapper_handle_trie_size() { return this->mapper->handle_trie_size(); }
