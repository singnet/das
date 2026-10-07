#pragma once

#include "JsonConfig.h"

using namespace std;

namespace atomdb {

class AtomDBInitializer {
   public:
    ~AtomDBInitializer() {}
    static void init(const commons::JsonConfig& atomdb_config);
};

}  // namespace atomdb
