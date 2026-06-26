#pragma once

#include <memory>
#include <mutex>

#include "CommandRouterHttpAPI.h"
#include "BusCommandProcessor.h"
#include "JsonConfig.h"

using namespace std;
using namespace service_bus;

namespace command_router {

class CommandRouterHttpAPISingleton {
   public:
    static void init(const JsonConfig& command_router_config,
                     const shared_ptr<BusCommandProcessor>& processor);
    static shared_ptr<CommandRouterHttpAPI> get_instance();
    static void provide(shared_ptr<CommandRouterHttpAPI> http_api);

   private:
    CommandRouterHttpAPISingleton() = default;

    /** @brief Build and start HTTP API from command_router.http_api config. Sets HTTP_API and
     * INITIALIZED. */
    static void create_and_start(const JsonConfig& command_router_config,
                                 const shared_ptr<BusCommandProcessor>& processor);

    /** @brief Stop and clear HTTP_API after a failed startup. */
    static void shutdown_on_startup_failure();

    static bool INITIALIZED;
    static shared_ptr<CommandRouterHttpAPI> HTTP_API;
    static mutex API_MUTEX;
};

}  // namespace command_router
