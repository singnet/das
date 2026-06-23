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
      max_events(max_events) {}

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

void CommandExecution::publish_event(const json& payload) {
    if (this->events.size() >= this->max_events && !this->is_terminal(this->status)) {
        RAISE_ERROR("Execution event buffer full");
    }
    this->events.push_back(payload.dump());
    this->cv.notify_all();
}

json CommandExecution::lifecycle_event() const {
    return {{"execution_id", this->execution_id}, {"status", this->status_to_string(this->status)}};
}

void CommandExecution::mark_running() {
    this->status = ExecutionStatus::RUNNING;
    this->publish_event(this->lifecycle_event());
}

void CommandExecution::publish_chunk(int seq, const vector<string>& data) {
    this->received_count += static_cast<int>(data.size());
    this->publish_event({{"execution_id", this->execution_id},
                         {"type", "chunk"},
                         {"seq", seq},
                         {"data", data},
                         {"received_count", this->received_count}});
}

void CommandExecution::stamp_finished_at() {
    if (this->finished_at_ms == 0) {
        this->finished_at_ms = Utils::get_current_time_millis();
    }
}

void CommandExecution::mark_completed(long long duration_ms, int total_items) {
    this->duration_ms = duration_ms;
    this->total_items = total_items;
    this->status = ExecutionStatus::COMPLETED;
    this->stamp_finished_at();
    auto event = this->lifecycle_event();
    event["duration_ms"] = duration_ms;
    event["total_items"] = total_items;
    this->publish_event(event);
}

void CommandExecution::mark_error(const string& message) {
    this->error_message = message;
    this->status = ExecutionStatus::ERROR;
    this->stamp_finished_at();
    auto event = this->lifecycle_event();
    event["message"] = message;
    this->publish_event(event);
}

void CommandExecution::mark_aborted() {
    this->status = ExecutionStatus::ABORTED;
    this->stamp_finished_at();
    this->publish_event(this->lifecycle_event());
}
