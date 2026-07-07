#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
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
 * Thread-safe: callers do not need to lock mtx; public methods synchronize internally.
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

    ExecutionStatus status() const;
    string status_string() const;
    bool is_terminal_status() const;
    bool is_cancel_requested() const;
    int received_count() const;
    long long finished_at_ms() const;

    /** @brief JSON body for GET /executions/{id}. */
    json to_status_json() const;

    /**
     * @brief Request cancellation.
     * @return false if already terminal; sets already_terminal when non-null.
     */
    bool try_request_cancel(ExecutionStatus* already_terminal = nullptr);

    /**
     * @brief Wait for the next stream event or terminal state.
     * @return payload when next_index points at a new event; nullopt on timeout or terminal drain.
     */
    optional<string> wait_next_event(size_t& next_index,
                                     chrono::milliseconds timeout,
                                     bool& stream_finished);

    /** @brief True when terminal and finished_at_ms is older than retention_ms. */
    bool is_retention_expired(long long now_ms, long long retention_ms) const;

    /** @brief Append a chunk event and update received_count. */
    void publish_chunk(int seq, const vector<string>& data);

    /** @brief PENDING -> RUNNING; emits a lifecycle event. */
    void mark_running();

    /** @brief -> COMPLETED; sets duration/totals and emits a lifecycle event. */
    void mark_completed(long long duration_ms, int total_items);

    /** @brief -> ERROR; sets error_message and emits a lifecycle event. */
    void mark_error(const string& message);

    /** @brief -> ABORTED; emits a lifecycle event. */
    void mark_aborted();

    void mark_error_unless_terminal(const string& message);
    void mark_aborted_unless_terminal();

   private:
    mutable mutex mtx_;
    condition_variable cv_;

    ExecutionStatus status_ = ExecutionStatus::PENDING;
    int received_count_ = 0;
    int total_items_ = 0;
    long long duration_ms_ = 0;
    long long finished_at_ms_ = 0;
    string error_message_;
    bool cancel_requested_ = false;
    vector<string> events_;

    void publish_event_locked(const json& payload);
    void stamp_finished_at_locked();
    json lifecycle_event_locked() const;
};

}  // namespace command_router
