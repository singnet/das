#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "DedicatedThread.h"
#include "JsonConfig.h"
#include "Processor.h"
#include "httplib.h"
#include "processor/ThreadPool.h"

using namespace std;
using namespace commons;

namespace command_router {

/**
 * @brief Embedded HTTP + WebSocket server for asynchronous command execution.
 *
 * Exposes the command-router dashboard API:
 *   POST  /command-router/executions — create a new execution
 *   GET   /command-router/executions/{id} — inspect execution state
 *   POST  /command-router/executions/{id}/cancel — request cancellation
 *   WS    /command-router/ws/{id} — stream execution events
 *
 *   This class implements ThreadMethod so it is driven by a DedicatedThread.
 *   setup() binds the port synchronously with bind_to_port(); thread_one_step()
 *   then calls listen_after_bind(), which blocks until stop() is called.
 *   Background executions are dispatched to an internal ThreadPool so that the
 *   HTTP listener thread is never blocked.
 *
 *  * Memory management:
 *   Execution state is kept in memory. As soon as an execution reaches a
 *   terminal state (completed / error / aborted) it is removed from memory
 *   by cleanup_finished_executions().
 */
class CommandRouterHttpAPI : public processor::Processor, public processor::ThreadMethod {
   public:
    /**
     * @param host Address to bind.
     * @param port Port to listen on.
     * @param thread_pool Shared pointer to the thread pool for executing commands.
     */
    CommandRouterHttpAPI(const string& host, int port, shared_ptr<processor::ThreadPool> thread_pool);

    /** @brief calls stop() */
    ~CommandRouterHttpAPI() override;

    /**
     * @brief Initialises and starts the HTTP server within the project's Processor lifecycle.
     *
     * Binds each entry in additional_subprocessors to this instance via Processor::bind_subprocessor()
     *
     * @param instance                 The CommandRouterHttpAPI instance to initialise.
     * @param additional_subprocessors Subprocessors to bind before starting
     */
    static void initialize(shared_ptr<CommandRouterHttpAPI> instance,
                           vector<shared_ptr<processor::Processor>> additional_subprocessors);

    /**
     * @brief DedicatedThread entry point — enters the blocking listen_after_bind() loop.
     *
     * @return false after the listening loop exits.
     */
    bool thread_one_step() override;

    /** @brief Register routes, bind the port, then Processor::setup(). */
    void setup() override;

    /**
     * @brief Request graceful shutdown of the HTTP listening loop.
     */
    void stop() override;

   private:
    /**
     * @brief In-memory state for one command execution.
     *
     * All fields are protected by mtx.
     * cv is notified whenever a new event is appended so that the WebSocket
     * handler wakes up and forwards it to the connected client.
     */
    struct Execution {
        string execution_id;
        string command_type;
        string command_text;

        mutex mtx;
        condition_variable cv;

        string status = "pending";  // pending|running|completed|error|aborted
        int received_count = 0;
        int total_items = 0;
        long long duration_ms = 0;
        string error_message;
        bool cancel_requested = false;

        vector<string> events;
    };

    string host;
    int port;
    httplib::Server server;
    shared_ptr<processor::ThreadPool> thread_pool;

    mutex executions_mutex;
    unordered_map<string, shared_ptr<Execution>> executions;
    unsigned int next_execution_number{1};

    // -------------------------------------------------------------------------
    // Route setup
    // -------------------------------------------------------------------------

    /**
     * @brief Register all HTTP and WebSocket routes on server.
     */
    void setup_routes();

    // -------------------------------------------------------------------------
    // Execution helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Look up an execution by id.
     */
    shared_ptr<Execution> find_execution(const string& execution_id);

    /**
     * @brief Run one command asynchronously (executed inside thread_pool).
     *
     * Publishes events to exec->events and notifies exec->cv after each append
     * so that the WebSocket handler wakes up and forwards the event to the client.
     */
    void run_execution(const shared_ptr<Execution>& exec);

    /**
     * @brief Append a serialised JSON event to exec->events and notify cv.
     */
    static void append_event_locked(const shared_ptr<Execution>& exec, const JsonConfig& payload);

    /**
     * @brief Remove all executions in a terminal state (completed / error / aborted).
     */
    void cleanup_finished_executions();

    /**
     * @brief Generate a unique id.
     */
    string generate_execution_id();

    // -------------------------------------------------------------------------
    // Event factory helpers
    // -------------------------------------------------------------------------

    static JsonConfig started_event(const string& execution_id);
    static JsonConfig chunk_event(const string& execution_id,
                                  int seq,
                                  const vector<string>& data,
                                  int received_count);
    static JsonConfig completed_event(const string& execution_id,
                                      long long duration_ms,
                                      int total_items);
    static JsonConfig error_event(const string& execution_id, const string& message);
    static JsonConfig aborted_event(const string& execution_id);

    void set_json_response(httplib::Response& res, int status_code, const JsonConfig& payload);
};

}  // namespace command_router