#include "CommandRouterHttpAPISingleton.h"

#include "Utils.h"

using namespace command_router;
using namespace commons;

bool CommandRouterHttpAPISingleton::INITIALIZED = false;
shared_ptr<CommandRouterHttpAPI> CommandRouterHttpAPISingleton::HTTP_API =
    shared_ptr<CommandRouterHttpAPI>(nullptr);
mutex CommandRouterHttpAPISingleton::API_MUTEX;

namespace {

pair<string, int> parse_http_api_endpoint(const JsonConfig& command_router_config) {
    auto endpoint = command_router_config.at_path("http_api.endpoint").get<string>();
    auto tokens = Utils::split(endpoint, ':');
    if (tokens.size() != 2) {
        RAISE_ERROR(
            "Invalid command_router.http_api configuration: endpoint must be in the format "
            "<hostname>:<port>");
    }

    int port;
    try {
        port = stoi(tokens[1]);
    } catch (const exception&) {
        RAISE_ERROR("Invalid command_router.http_api configuration: port must be an integer");
    }

    return {tokens[0], port};
}

}  // namespace

void CommandRouterHttpAPISingleton::init(const JsonConfig& command_router_config) {
    lock_guard<mutex> semaphore(API_MUTEX);
    if (INITIALIZED) {
        RAISE_ERROR(
            "CommandRouterHttpAPISingleton already initialized. "
            "CommandRouterHttpAPISingleton::init() should be called only once.");
    }
    CommandRouterHttpAPISingleton::create_and_start(command_router_config);
}

void CommandRouterHttpAPISingleton::create_and_start(const JsonConfig& command_router_config) {
    auto [host, port] = parse_http_api_endpoint(command_router_config);

    unsigned int num_threads =
        command_router_config.at_path("http_api.thread_pool_size").get_or<unsigned int>(4);

    auto thread_pool_executor =
        make_shared<processor::ThreadPool>("http_api_thread_pool_executor", num_threads);
    HTTP_API = make_shared<CommandRouterHttpAPI>(host, port, thread_pool_executor);

    auto http_api_thread = make_shared<processor::DedicatedThread>("http_api_thread", HTTP_API.get());

    try {
        CommandRouterHttpAPI::initialize(HTTP_API, {http_api_thread, thread_pool_executor});
        Utils::sleep(200);  // time to bind the port
        INITIALIZED = true;
    } catch (const std::exception& e) {
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
