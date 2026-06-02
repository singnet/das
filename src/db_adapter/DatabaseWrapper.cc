#include "DatabaseWrapper.h"

#include "DatabaseMapper.h"

using namespace std;
using namespace db_adapter;

DatabaseWrapper::DatabaseWrapper(shared_ptr<DatabaseConnection> db_conn,
                                 shared_ptr<DatabaseMapper> mapper)
    : db_conn(db_conn), mapper(mapper) {}

unsigned int DatabaseWrapper::mapper_handle_trie_size() { return this->mapper->handle_trie_size(); }
