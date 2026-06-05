#pragma once

#include <memory>
#include <mutex>

#include "JsonConfig.h"
#include "SystemParameters.h"

using namespace std;

namespace commons {

class SystemParametersSingleton {
   public:
    static void init(const JsonConfig& das_config);
    static void init_from_file(const string& path);
    static shared_ptr<SystemParameters> get_instance();
    static void provide(shared_ptr<SystemParameters> parameters);

   private:
    SystemParametersSingleton() = default;
    static bool INITIALIZED;
    static shared_ptr<SystemParameters> PARAMS;
    static mutex API_MUTEX;
};

}  // namespace commons
