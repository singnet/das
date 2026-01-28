#include "DBWrapper.h"

#include <stdexcept>

template <typename ConnT>
DatabaseWrapper<ConnT>::DatabaseWrapper(shared_ptr<Mapper> mapper) : mapper(move(mapper)) {}

template <typename ConnT>
SQLWrapper<ConnT>::SQLWrapper(db_adapter_types::MAPPER_TYPE mapper_type)
    : DatabaseWrapper<ConnT>(create_mapper(mapper_type)) {}

template <typename ConnT>
shared_ptr<Mapper> SQLWrapper<ConnT>::create_mapper(db_adapter_types::MAPPER_TYPE mapper_type) {
    switch (mapper_type) {
        case db_adapter_types::MAPPER_TYPE::SQL2METTA:
            return make_shared<SQL2MettaMapper>();
        case db_adapter_types::MAPPER_TYPE::SQL2ATOMS:
            return make_shared<SQL2AtomsMapper>();
        default:
            throw invalid_argument("Unknown mapper type");
    }
}