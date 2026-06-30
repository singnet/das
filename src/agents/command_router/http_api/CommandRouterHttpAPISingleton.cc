#include "CommandRouterHttpAPISingleton.h"

#include "BusCommandRouterProcessor.h"
#include "CommandRouterHttpAPIConfig.h"

using namespace command_router;
using namespace commons;

bool CommandRouterHttpAPISingleton::INITIALIZED = false;
shared_ptr<CommandRouterHttpAPI> CommandRouterHttpAPISingleton::HTTP_API =
    shared_ptr<CommandRouterHttpAPI>(nullptr);
mutex CommandRouterHttpAPISingleton::API_MUTEX;

void CommandRouterHttpAPISingleton::init(const JsonConfig& command_router_config,
                                         const shared_ptr<BusCommandProcessor>& processor) {
    lock_guard<mutex> semaphore(API_MUTEX);
    if (INITIALIZED) {
        RAISE_ERROR(
            "CommandRouterHttpAPISingleton already initialized. "
            "CommandRouterHttpAPISingleton::init() should be called only once.");
    }
    create_and_start(command_router_config, processor);
}

void CommandRouterHttpAPISingleton::create_and_start(const JsonConfig& command_router_config,
                                                     const shared_ptr<BusCommandProcessor>& processor) {
    auto router_processor = dynamic_pointer_cast<BusCommandRouterProcessor>(processor);

    if (router_processor == nullptr) {
        RAISE_ERROR("CommandRouterHttpAPISingleton requires a non-null BusCommandRouterProcessor");
    }

    const auto config = CommandRouterHttpAPIConfig::from_config(command_router_config);

    auto thread_pool_executor =
        make_shared<processor::ThreadPool>("http_api_thread_pool_executor", config.thread_pool_size);

    HTTP_API = make_shared<CommandRouterHttpAPI>(config.host,
                                                 config.port,
                                                 thread_pool_executor,
                                                 router_processor,
                                                 config.settings,
                                                 config.bus_host);

    auto http_api_thread = make_shared<processor::DedicatedThread>("http_api_thread", HTTP_API.get());

    try {
        CommandRouterHttpAPI::initialize(HTTP_API, {http_api_thread, thread_pool_executor});
        INITIALIZED = true;
    } catch (const exception& e) {
        CommandRouterHttpAPISingleton::shutdown_on_startup_failure();
        RAISE_ERROR(
            "CommandRouterHttpAPISingleton::init(): CommandRouterHttpAPI::initialize() failed: " +
            string(e.what()));
    }
}

void CommandRouterHttpAPISingleton::shutdown_on_startup_failure() {
    if (HTTP_API && HTTP_API->is_running()) {
        HTTP_API->stop();
    }
    HTTP_API = shared_ptr<CommandRouterHttpAPI>(nullptr);
}

shared_ptr<CommandRouterHttpAPI> CommandRouterHttpAPISingleton::get_instance() {
    lock_guard<mutex> semaphore(API_MUTEX);
    if (!INITIALIZED || HTTP_API == nullptr) {
        RAISE_ERROR("CommandRouterHttpAPISingleton not initialized");
        return shared_ptr<CommandRouterHttpAPI>{nullptr};
    }
    return HTTP_API;
}

void CommandRouterHttpAPISingleton::provide(shared_ptr<CommandRouterHttpAPI> http_api) {
    lock_guard<mutex> semaphore(API_MUTEX);
    HTTP_API = http_api;
    INITIALIZED = (http_api != nullptr);
}
