#include "CommandExecution.h"

using namespace std;
using namespace commons;
using namespace command_router;

CommandExecution::CommandExecution(const string& execution_id,
                                   const string& command_type,
                                   const string& command_text,
                                   size_t max_events)
    : execution_id(execution_id),
      command_type(command_type),
      command_text(command_text),
      max_events(max_events) {
    if (this->max_events == 0) {
        RAISE_ERROR("max_events must be greater than 0");
    }
}

string CommandExecution::status_to_string(ExecutionStatus status) {
    switch (status) {
        case ExecutionStatus::PENDING:
            return "pending";
        case ExecutionStatus::RUNNING:
            return "running";
        case ExecutionStatus::COMPLETED:
            return "completed";
        case ExecutionStatus::ERROR:
            return "error";
        case ExecutionStatus::ABORTED:
            return "aborted";
    }
    RAISE_ERROR("Invalid execution status");
}

bool CommandExecution::is_terminal(ExecutionStatus status) {
    return status == ExecutionStatus::COMPLETED || status == ExecutionStatus::ERROR ||
           status == ExecutionStatus::ABORTED;
}

ExecutionStatus CommandExecution::status() const {
    lock_guard<mutex> lock(this->mtx_);
    return this->status_;
}

string CommandExecution::status_string() const { return status_to_string(this->status()); }

bool CommandExecution::is_terminal_status() const { return is_terminal(this->status()); }

bool CommandExecution::is_cancel_requested() const {
    lock_guard<mutex> lock(this->mtx_);
    return this->cancel_requested_;
}

int CommandExecution::received_count() const {
    lock_guard<mutex> lock(this->mtx_);
    return this->received_count_;
}

long long CommandExecution::finished_at_ms() const {
    lock_guard<mutex> lock(this->mtx_);
    return this->finished_at_ms_;
}

json CommandExecution::to_status_json() const {
    lock_guard<mutex> lock(this->mtx_);
    json body = {{"execution_id", this->execution_id},
                 {"status", status_to_string(this->status_)},
                 {"received_count", this->received_count_},
                 {"total_items", this->total_items_},
                 {"duration_ms", this->duration_ms_}};
    if (!this->error_message_.empty()) {
        body["error_message"] = this->error_message_;
    }
    return body;
}

bool CommandExecution::try_request_cancel(ExecutionStatus* already_terminal) {
    lock_guard<mutex> lock(this->mtx_);
    if (is_terminal(this->status_)) {
        if (already_terminal != nullptr) {
            *already_terminal = this->status_;
        }
        return false;
    }
    this->cancel_requested_ = true;
    this->cv_.notify_all();
    return true;
}

optional<string> CommandExecution::wait_next_event(size_t& next_index,
                                                   chrono::milliseconds timeout,
                                                   bool& stream_finished) {
    unique_lock<mutex> lock(this->mtx_);
    stream_finished = false;

    const bool ready = this->cv_.wait_for(
        lock, timeout, [&] { return next_index < this->events_.size() || is_terminal(this->status_); });

    if (!ready) {
        return nullopt;
    }

    if (next_index >= this->events_.size()) {
        stream_finished = is_terminal(this->status_);
        return nullopt;
    }

    string payload = this->events_[next_index++];
    stream_finished = is_terminal(this->status_) && next_index >= this->events_.size();
    return payload;
}

bool CommandExecution::is_retention_expired(long long now_ms, long long retention_ms) const {
    lock_guard<mutex> lock(this->mtx_);
    return is_terminal(this->status_) && this->finished_at_ms_ > 0 &&
           (now_ms - this->finished_at_ms_) > retention_ms;
}

void CommandExecution::publish_event_locked(const json& payload) {
    if (this->events_.size() >= this->max_events && !is_terminal(this->status_)) {
        RAISE_ERROR("Execution event buffer full");
    }
    this->events_.push_back(payload.dump());
    this->cv_.notify_all();
}

json CommandExecution::lifecycle_event_locked() const {
    return {{"execution_id", this->execution_id}, {"status", status_to_string(this->status_)}};
}

void CommandExecution::stamp_finished_at_locked() {
    if (this->finished_at_ms_ == 0) {
        this->finished_at_ms_ = Utils::get_current_time_millis();
    }
}

void CommandExecution::mark_running() {
    lock_guard<mutex> lock(this->mtx_);
    this->status_ = ExecutionStatus::RUNNING;
    this->publish_event_locked(this->lifecycle_event_locked());
}

void CommandExecution::publish_chunk(int seq, const vector<string>& data) {
    lock_guard<mutex> lock(this->mtx_);
    this->received_count_ += static_cast<int>(data.size());
    this->publish_event_locked({{"execution_id", this->execution_id},
                                {"type", "chunk"},
                                {"seq", seq},
                                {"data", data},
                                {"received_count", this->received_count_}});
}

void CommandExecution::mark_completed(long long duration_ms, int total_items) {
    lock_guard<mutex> lock(this->mtx_);
    this->duration_ms_ = duration_ms;
    this->total_items_ = total_items;
    this->status_ = ExecutionStatus::COMPLETED;
    this->stamp_finished_at_locked();
    auto event = this->lifecycle_event_locked();
    event["duration_ms"] = duration_ms;
    event["total_items"] = total_items;
    this->publish_event_locked(event);
}

void CommandExecution::mark_error(const string& message) {
    lock_guard<mutex> lock(this->mtx_);
    this->error_message_ = message;
    this->status_ = ExecutionStatus::ERROR;
    this->stamp_finished_at_locked();
    auto event = this->lifecycle_event_locked();
    event["message"] = message;
    this->publish_event_locked(event);
}

void CommandExecution::mark_aborted() {
    lock_guard<mutex> lock(this->mtx_);
    this->status_ = ExecutionStatus::ABORTED;
    this->stamp_finished_at_locked();
    this->publish_event_locked(this->lifecycle_event_locked());
}

void CommandExecution::mark_error_unless_terminal(const string& message) {
    lock_guard<mutex> lock(this->mtx_);
    if (is_terminal(this->status_)) {
        return;
    }
    this->error_message_ = message;
    this->status_ = ExecutionStatus::ERROR;
    this->stamp_finished_at_locked();
    auto event = this->lifecycle_event_locked();
    event["message"] = message;
    this->publish_event_locked(event);
}

void CommandExecution::mark_aborted_unless_terminal() {
    lock_guard<mutex> lock(this->mtx_);
    if (is_terminal(this->status_)) {
        return;
    }
    this->status_ = ExecutionStatus::ABORTED;
    this->stamp_finished_at_locked();
    this->publish_event_locked(this->lifecycle_event_locked());
}
