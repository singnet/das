#pragma once

#include <string>

#include "CommandExecution.h"
#include "JsonConfig.h"

using namespace std;
using namespace commons;

namespace command_router {

struct HttpAPISettings {
    size_t max_concurrent_executions = 100;
    size_t max_queued_executions = 500;
    size_t max_events_per_execution = CommandExecution::DEFAULT_MAX_EVENTS;
    long long execution_retention_ms = 15 * 60 * 1000;
    /** Max query/evolution answers per WebSocket chunk event (1 = one item per event). */
    size_t stream_items_per_chunk = 1;
};

struct CommandRouterHttpAPIConfig {
    string host;
    int port = 0;
    /** Hostname of the command router bus node (from command_router.endpoint). */
    string bus_host;
    HttpAPISettings settings;
    unsigned int thread_pool_size = 4;

    static CommandRouterHttpAPIConfig from_config(const JsonConfig& command_router_config);
};

}  // namespace command_router
