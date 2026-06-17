#pragma once

#include <memory>
#include <mutex>

#include "CommandRouterHttpAPI.h"
#include "JsonConfig.h"

using namespace std;

namespace command_router {

class CommandRouterHttpAPISingleton {
   public:
    static void init(const JsonConfig& command_router_config);
    static shared_ptr<CommandRouterHttpAPI> get_instance();
    static void provide(shared_ptr<CommandRouterHttpAPI> http_api);

   private:
    CommandRouterHttpAPISingleton() = default;
    static bool INITIALIZED;
    static shared_ptr<CommandRouterHttpAPI> HTTP_API;
    static mutex API_MUTEX;
};

}  // namespace command_router
