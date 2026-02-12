#include "DatabaseWrapper.h"

namespace {
shared_ptr<Mapper> create_mapper(MAPPER_TYPE mapper_type) {
    switch (mapper_type) {
        case MAPPER_TYPE::SQL2METTA:
            return make_shared<SQL2MettaMapper>();
        case MAPPER_TYPE::SQL2ATOMS:
            return make_shared<SQL2AtomsMapper>();
        default:
            throw invalid_argument("Unknown mapper type");
    }
}
}  // namespace

SQLWrapper::SQLWrapper(MAPPER_TYPE mapper_type)
    : DatabaseWrapper(create_mapper(mapper_type), mapper_type) {}

DatabaseWrapper::DatabaseWrapper(shared_ptr<Mapper> mapper, MAPPER_TYPE mapper_type)
    : mapper(move(mapper)), mapper_type(mapper_type) {}

unsigned int DatabaseWrapper::mapper_handle_trie_size() { return this->mapper->handle_trie_size(); }
