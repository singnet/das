#include "CommandRouterHttpAPISingleton.h"

using namespace command_router;

bool CommandRouterHttpAPISingleton::INITIALIZED = false;
shared_ptr<CommandRouterHttpAPI> CommandRouterHttpAPISingleton::HTTP_API =
    shared_ptr<CommandRouterHttpAPI>(nullptr);
mutex CommandRouterHttpAPISingleton::API_MUTEX;

void CommandRouterHttpAPISingleton::init(const JsonConfig& command_router_config) {
    lock_guard<mutex> semaphore(API_MUTEX);
    if (INITIALIZED) {
        RAISE_ERROR(
            "CommandRouterHttpAPISingleton already initialized. "
            "CommandRouterHttpAPISingleton::init() should be called only once.");
    } else {
        auto host = command_router_config.at_path("http_api_host").get<string>();
        auto port = command_router_config.at_path("http_api_port").get<int>();

        // TODO: Change to get the number of threads on the machine.
        unsigned int num_threads = 4;

        auto thread_pool_executor =
            make_shared<ThreadPool>("http_api_thread_pool_executor", num_threads);

        HTTP_API = make_shared<CommandRouterHttpAPI>(host, port, thread_pool_executor);

        auto http_api_thread = make_shared<DedicatedThread>("http_api_thread", HTTP_API.get());

        try {
            CommandRouterHttpAPI::initialize(HTTP_API, {thread_pool_executor, http_api_thread});
            Utils::sleep(200);  // time to bind the port
            INITIALIZED = true;
        } catch (const std::exception& e) {
            if (HTTP_API && HTTP_API->is_running()) {
                HTTP_API->stop();
            }
            HTTP_API = shared_ptr<CommandRouterHttpAPI>(nullptr);
            RAISE_ERROR(
                "CommandRouterHttpAPISingleton::init(): CommandRouterHttpAPI::initialize() failed: " +
                string(e.what()));
        }
    }
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
    if (http_api == nullptr) {
        RAISE_ERROR("CommandRouterHttpAPISingleton::provide(): http_api cannot be nullptr");
    }
    lock_guard<mutex> semaphore(API_MUTEX);
    HTTP_API = http_api;
    INITIALIZED = true;
}
