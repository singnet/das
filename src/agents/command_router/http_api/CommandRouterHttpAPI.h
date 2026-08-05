#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "BusCommandRouterProxyStreamPoller.h"
#include "CommandExecution.h"
#include "CommandRouterHttpAPIConfig.h"
#include "DedicatedThread.h"
#include "Processor.h"
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "processor/ThreadPool.h"

using namespace std;
using namespace commons;

using json = nlohmann::json;

namespace command_router {

class BusCommandRouterProcessor;

/**
 * @brief HTTP + WebSocket server for command execution.
 *
 * Routes:
 *   POST /command-router/executions          — schedule a command ({command, params})
 *   GET  /command-router/executions/{id}     — poll status
 *   POST /command-router/executions/{id}/cancel — request cancel
 *   WS   /command-router/ws/{id}             — stream JSON events
 *
 * The HTTP layer only unpacks the top-level envelope {command, params}. Command-specific
 * interpretation of params is delegated to the matching command handler (e.g. query).
 * WebSocket stream events use the same {command, params} envelope.
 *
 * Runs on a DedicatedThread: thread_one_step() blocks in listen() until stop().
 * Each accepted command is enqueued on thread_pool so the listener stays free.
 * Terminal executions stay in executions until execution_retention_ms expires.
 */
class CommandRouterHttpAPI : public processor::Processor, public processor::ThreadMethod {
   public:
    static const unordered_set<string> VALID_COMMAND;

    /**
     * @brief Construct a CommandRouterHttpAPI.
     * @param host        Hostname to bind.
     * @param port        Port to bind.
     * @param thread_pool Runs command work off the HTTP listener thread.
     * @param settings    Queue, event-buffer, and retention limits.
     */
    CommandRouterHttpAPI(const string& host,
                         int port,
                         shared_ptr<processor::ThreadPool> thread_pool,
                         shared_ptr<BusCommandRouterProcessor> router_processor,
                         HttpAPISettings settings = {},
                         const string& bus_host = "localhost");

    /** @brief Calls stop(). */
    ~CommandRouterHttpAPI() override;

    /**
     * @brief Bind subprocessors and start the Processor lifecycle.
     * Typically called once with the DedicatedThread and thread_pool as subprocessors.
     */
    static void initialize(shared_ptr<CommandRouterHttpAPI> instance,
                           vector<shared_ptr<processor::Processor>> additional_subprocessors);

    /** @brief DedicatedThread entry point; blocks in server.listen() until stop(). */
    bool thread_one_step() override;

    /** @brief Register routes, bind the port, then Processor::setup(). */
    void setup() override;

    /** @brief Stop listen(), drop expired executions, and stop the Processor. */
    void stop() override;

   private:
    string host;
    int port;
    httplib::Server server;
    shared_ptr<processor::ThreadPool> thread_pool;
    shared_ptr<BusCommandRouterProcessor> router_processor;
    string bus_host;
    string http_requestor_id;
    HttpAPISettings settings;
    atomic<bool> shutting_down{false};
    unordered_map<string, shared_ptr<CommandExecution>> executions;
    mutex executions_mtx;
    size_t pending_executions = 0;

    /** @brief Register all /command-router/ HTTP and WebSocket handlers. */
    void setup_routes();

    shared_ptr<CommandExecution> find_execution(const string& execution_id);

    /** @brief Thread-pool entry point; catches errors and marks the execution failed. */
    void run_execution(const shared_ptr<CommandExecution>& exec);

    /** @brief Run command/params and publish chunk/lifecycle events. */
    void run_execution_inner(const shared_ptr<CommandExecution>& exec);

    /** @brief Dispatch a router command and poll its response stream. */
    PollStreamResult execute_router_command(const string& command,
                                            const json& params,
                                            const function<bool()>& should_abort,
                                            const function<void(const json& chunk)>& on_chunk,
                                            const function<void(const string& error)>& on_error,
                                            const function<void()>& on_aborted);

    /** @brief Remove finished executions from the executions map. */
    void cleanup_finished_executions();

    string generate_execution_id();

    /** @brief Check if the command is valid. (Allowed values: query) */
    bool is_valid_command(const string& command) const;

    /** @brief Check queue limit and register a pending execution. */
    enum class AdmitResult { Admitted, QueueFull };
    AdmitResult try_admit_execution(const shared_ptr<CommandExecution>& exec);

    /** @brief Decrement pending_executions after an execution leaves the in-flight set. */
    void release_pending_execution();

    /** @brief Set response status and JSON body. */
    void set_json_response(httplib::Response& res, int status_code, const json& payload);
};

}  // namespace command_router
