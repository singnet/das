#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "CommandExecution.h"
#include "DedicatedThread.h"
#include "JsonConfig.h"
#include "Processor.h"
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "processor/ThreadPool.h"

using namespace std;
using namespace commons;

using json = nlohmann::json;

namespace command_router {

struct HttpAPISettings {
    size_t max_concurrent_executions = 100;
    size_t max_queued_executions = 500;
    size_t max_events_per_execution = CommandExecution::DEFAULT_MAX_EVENTS;
    long long execution_retention_ms = 15 * 60 * 1000;
};

/**
 * @brief HTTP + WebSocket server for asynchronous command execution.
 *
 * Routes:
 *   POST /command-router/executions          — schedule a command
 *   GET  /command-router/executions/{id}     — poll status
 *   POST /command-router/executions/{id}/cancel — request cancel
 *   WS   /command-router/ws/{id}             — stream JSON events
 *
 * Runs on a DedicatedThread: thread_one_step() blocks in listen() until stop().
 * Each accepted command is enqueued on thread_pool so the listener stays free.
 * Terminal executions stay in executions until execution_retention_ms expires.
 */
class CommandRouterHttpAPI : public processor::Processor, public processor::ThreadMethod {
   public:
    static const unordered_set<string> VALID_COMMAND_TYPES;

    /**
     * @brief Construct a CommandRouterHttpAPI.
     * @param host        Hostname to bind.
     * @param port        Port to bind.
     * @param thread_pool Runs command work off the HTTP listener thread.
     * @param settings    Concurrency, queue, event-buffer, and retention limits.
     */
    CommandRouterHttpAPI(const string& host,
                         int port,
                         shared_ptr<processor::ThreadPool> thread_pool,
                         HttpAPISettings settings = {});

    /** @brief Calls stop(). */
    ~CommandRouterHttpAPI() override;

    /**
     * @brief Bind subprocessors and start the Processor lifecycle.
     * Typically called once with the DedicatedThread and thread_pool as subprocessors.
     */
    static void initialize(shared_ptr<CommandRouterHttpAPI> instance,
                           vector<shared_ptr<processor::Processor>> additional_subprocessors);

    /** @brief Load HttpAPISettings from command_router.http_api.* config paths. */
    static HttpAPISettings settings_from_config(const JsonConfig& command_router_config);

    /** @brief DedicatedThread entry point; blocks in server.listen() until stop(). */
    bool thread_one_step() override;

    /** @brief Register routes, bind the port, then Processor::setup(). */
    void setup() override;

    /** @brief Register routes, then Processor::setup(). */
    void setup() override;

    /** @brief Stop listen(), drop expired executions, and stop the Processor. */
    void stop() override;

   private:
    string host;
    int port;
    httplib::Server server;
    shared_ptr<processor::ThreadPool> thread_pool;
    HttpAPISettings settings;

    /** Set while shutting down; new POST /executions return 503. */
    atomic<bool> shutting_down{false};

    mutex executions_mutex;

    unordered_map<string, shared_ptr<CommandExecution>> executions;

    /** @brief Register all /command-router/* HTTP and WebSocket handlers. */
    void setup_routes();

    shared_ptr<CommandExecution> find_execution(const string& execution_id);

    /** @brief Thread-pool entry point; catches errors and marks the execution failed. */
    void run_execution(const shared_ptr<CommandExecution>& exec);

    /** @brief Run command_type/command_text and publish chunk/lifecycle events. */
    void run_execution_inner(const shared_ptr<CommandExecution>& exec);

    void cleanup_finished_executions();

    string generate_execution_id();

    bool is_valid_command_type(const string& command_type) const;

    /**
     * @brief Check limits and reserve a slot in executions under executions_mutex.
     * Caller must hold no locks. Returns false if concurrent or queue limit is hit.
     */
    bool try_admit_execution(const shared_ptr<CommandExecution>& exec, size_t& active_count);

    /** @brief Count non-terminal executions; caller must hold executions_mutex. */
    size_t count_active_executions_locked() const;

    /** @brief Set response status and JSON body. */
    void set_json_response(httplib::Response& res, int status_code, const json& payload);
};

}  // namespace command_router
