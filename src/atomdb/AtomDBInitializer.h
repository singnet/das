#pragma once

#include "JsonConfig.h"

using namespace std;

namespace atomdb {

class AtomDBInitializer {
   public:
    ~AtomDBInitializer() {}
    /**
     * @brief Initializes the AtomDB singleton. May be called only once;
     */
    static void init(const commons::JsonConfig& atomdb_config);
};

}  // namespace atomdb
