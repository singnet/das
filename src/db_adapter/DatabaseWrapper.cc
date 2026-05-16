#include "DatabaseWrapper.h"

using namespace std;
using namespace db_adapter;

DatabaseWrapper::DatabaseWrapper(DatabaseConnection& db_client, MAPPER_TYPE mapper_type)
    : db_client(db_client), mapper(create_mapper(mapper_type)), mapper_type(mapper_type) {}

unsigned int DatabaseWrapper::mapper_handle_trie_size() { return this->mapper->handle_trie_size(); }

shared_ptr<Mapper> DatabaseWrapper::create_mapper(MAPPER_TYPE mapper_type) {
    switch (mapper_type) {
        case MAPPER_TYPE::SQL2ATOMS:
            return make_shared<SQL2AtomsMapper>();
        case MAPPER_TYPE::METTA2ATOMS:
            return make_shared<Metta2AtomsMapper>();
        default:
            throw invalid_argument("Unknown mapper type");
    }
}

SQLWrapper::SQLWrapper(DatabaseConnection& db_client, MAPPER_TYPE mapper_type)
    : DatabaseWrapper(db_client, mapper_type) {}
