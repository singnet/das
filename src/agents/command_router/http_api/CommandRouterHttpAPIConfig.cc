#include "CommandRouterHttpAPIConfig.h"

#include <tuple>

#include "Utils.h"

using namespace command_router;
using namespace commons;

static pair<string, int> parse_host_port(const string& endpoint) {
    string config_path = "command_router." + endpoint;
    auto tokens = Utils::split(endpoint, ':');
    if (tokens.size() != 2) {
        RAISE_ERROR("Invalid " + config_path + " configuration: endpoint must be <hostname>:<port>");
    }

    int port;
    try {
        port = stoi(tokens[1]);
    } catch (const exception&) {
        RAISE_ERROR("Invalid " + config_path + " configuration: port must be an integer");
    }

    return {tokens[0], port};
}

static HttpAPISettings load_http_api_settings(const JsonConfig& command_router_config) {
    HttpAPISettings settings;
    settings.max_concurrent_executions =
        command_router_config.at_path("http_api.max_concurrent_executions")
            .get_or<size_t>(settings.max_concurrent_executions);
    settings.max_queued_executions = command_router_config.at_path("http_api.max_queued_executions")
                                         .get_or<size_t>(settings.max_queued_executions);
    settings.max_events_per_execution =
        command_router_config.at_path("http_api.max_events_per_execution")
            .get_or<size_t>(settings.max_events_per_execution);
    settings.execution_retention_ms = command_router_config.at_path("http_api.execution_retention_ms")
                                          .get_or<long long>(settings.execution_retention_ms);
    settings.stream_items_per_chunk = command_router_config.at_path("http_api.stream_items_per_chunk")
                                          .get_or<size_t>(settings.stream_items_per_chunk);
    if (settings.stream_items_per_chunk == 0) {
        RAISE_ERROR("command_router.http_api.stream_items_per_chunk must be at least 1");
    }
    return settings;
}

CommandRouterHttpAPIConfig CommandRouterHttpAPIConfig::from_config(
    const JsonConfig& command_router_config) {
    CommandRouterHttpAPIConfig config;
    tie(config.host, config.port) =
        parse_host_port(command_router_config.at_path("http_api.endpoint").get<string>());
    config.settings = load_http_api_settings(command_router_config);
    config.thread_pool_size =
        command_router_config.at_path("http_api.thread_pool_size").get_or<unsigned int>(4);
    tie(config.bus_host, std::ignore) =
        parse_host_port(command_router_config.at_path("endpoint").get<string>());
    return config;
}
