#include "MapperFactory.h"

#include "Metta2AtomsMapper.h"
#include "Sql2AtomsMapper.h"
#include "Utils.h"

using namespace std;
using namespace db_adapter;

shared_ptr<DatabaseMapper> MapperFactory::create(MAPPER_TYPE mapper_type) {
    switch (mapper_type) {
        case MAPPER_TYPE::SQL2ATOMS:
            return make_shared<SQL2AtomsMapper>();
        case MAPPER_TYPE::METTA2ATOMS:
            return make_shared<Metta2AtomsMapper>();
        default:
            RAISE_ERROR("Unknown mapper type");
    }
}