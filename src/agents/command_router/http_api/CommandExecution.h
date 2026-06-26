#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#include "Utils.h"
#include "nlohmann/json.hpp"

using namespace std;

using json = nlohmann::json;

namespace command_router {

enum ExecutionStatus { PENDING, RUNNING, COMPLETED, ERROR, ABORTED };

/**
 * @brief In-memory state for one asynchronous command run.
 *
 * Holds status, progress counters, and a JSON event log (events) consumed by
 * GET /executions/{id} and WebSocket replay. Status transitions always emit a
 * lifecycle event so clients can rely on status in the stream.
 *
 * All mutable fields are protected by mtx. cv is notified on each new event.
 */
class CommandExecution {
   public:
    static constexpr size_t DEFAULT_MAX_EVENTS = 10000;

    CommandExecution(const string& execution_id,
                     const string& command_type,
                     const string& command_text,
                     size_t max_events = DEFAULT_MAX_EVENTS);
    ~CommandExecution() = default;

    /** @brief API-facing status string (pending, running, completed, error, aborted). */
    static string status_to_string(ExecutionStatus status);

    /** @brief True for completed, error, or aborted. */
    static bool is_terminal(ExecutionStatus status);

    string execution_id;
    string command_type;
    string command_text;
    size_t max_events;

    mutex mtx;
    condition_variable cv;

    ExecutionStatus status = ExecutionStatus::PENDING;
    int received_count = 0;
    int total_items = 0;
    long long duration_ms = 0;
    long long finished_at_ms = 0;
    string error_message;
    bool cancel_requested = false;

    /** Serialised JSON payloads, in order, for WebSocket replay. */
    vector<string> events;

    /**
     * @brief Append one serialised JSON event and notify cv.
     * Caller must hold mtx. Throws if the buffer is full while still running.
     */
    void publish_event(const json& payload);

    /** @brief Append a chunk event and update received_count */
    void publish_chunk(int seq, const vector<string>& data);

    /** @brief PENDING -> RUNNING; emits a lifecycle event */
    void mark_running();

    /** @brief -> COMPLETED; sets duration/totals and emits a lifecycle event. */
    void mark_completed(long long duration_ms, int total_items);

    /** @brief -> ERROR; sets error_message and emits a lifecycle event. */
    void mark_error(const string& message);

    /** @brief -> ABORTED; emits a lifecycle event (e.g. after cancel_requested). */
    void mark_aborted();

   private:
    /** @brief Set finished_at_ms once, on first transition to a terminal status. */
    void stamp_finished_at();

    /** @brief Base lifecycle payload: execution_id + current status. */
    json lifecycle_event() const;
};

}  // namespace command_router
