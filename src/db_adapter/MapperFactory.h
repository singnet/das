#pragma once

#include <memory>

#include "DatabaseMapper.h"
#include "DatabaseTypes.h"

using namespace std;

namespace db_adapter {

class MapperFactory {
   public:
    static shared_ptr<DatabaseMapper> create(MAPPER_TYPE mapper_type);
};

}  // namespace db_adapter