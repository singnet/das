#include "Sink.h"
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace query_element;

// ------------------------------------------------------------------------------------------------
// Constructors and destructors

Sink::Sink(shared_ptr<QueryElement> precedent, const string& id, bool setup_buffers_flag) {
    LOG_LOCAL_DEBUG("Creating Sink: " + std::to_string((unsigned long) this));
    this->precedent = precedent;
    this->id = id;
    this->query_answer_count = 0;
    this->input_buffer = nullptr;
    if (setup_buffers_flag) {
        LOG_LOCAL_DEBUG("Sink " + std::to_string((unsigned long) this) + " is setting up buffers...");
        setup_buffers();
        LOG_LOCAL_DEBUG("Sink " + std::to_string((unsigned long) this) + " is setting up buffers... Done");
    }
}

Sink::~Sink() { 
    LOG_LOCAL_DEBUG("Deleting Sink: " + std::to_string((unsigned long) this) + "...");
    this->input_buffer->graceful_shutdown(); 
    LOG_LOCAL_DEBUG("Deleting Sink: " + std::to_string((unsigned long) this) + "... Done");
}

// ------------------------------------------------------------------------------------------------
// Public methods

void Sink::setup_buffers() {
    LOG_LOCAL_DEBUG("Setting up buffers for Sink: " + std::to_string((unsigned long) this));
    if (this->subsequent_id != "") {
        Utils::error("Invalid non-empty subsequent id: " + this->subsequent_id);
    }
    if (this->id == "") {
        Utils::error("Invalid empty id");
    }
    this->input_buffer = make_shared<QueryNodeServer>(this->id);
    this->precedent->subsequent_id = this->id;
    LOG_LOCAL_DEBUG("Setting up precedent buffers for Sink: " + std::to_string((unsigned long) this) + "...");
    this->precedent->setup_buffers();
    LOG_LOCAL_DEBUG("Setting up precedent buffers for Sink: " + std::to_string((unsigned long) this) + "... Done");
}

void Sink::graceful_shutdown() {
    LOG_LOCAL_DEBUG("Gracefully shutting down Sink: " + std::to_string((unsigned long) this) + "...");
    if (is_flow_finished()) {
        LOG_LOCAL_DEBUG("Gracefully shutting down Sink: " + std::to_string((unsigned long) this) + "... Done");
        return;
    }
    LOG_LOCAL_DEBUG("Gracefully shutting down precedent of Sink: " + std::to_string((unsigned long) this) + "...");
    this->precedent->graceful_shutdown();
    LOG_LOCAL_DEBUG("Gracefully shutting down precedent of Sink: " + std::to_string((unsigned long) this) + "... Done");
    if (this->input_buffer != nullptr) {
        LOG_LOCAL_DEBUG("Gracefully shutting down input buffer of Sink: " + std::to_string((unsigned long) this) + "...");
        this->input_buffer->graceful_shutdown();
        LOG_LOCAL_DEBUG("Gracefully shutting down input buffer of Sink: " + std::to_string((unsigned long) this) + "... Done");
    }
    LOG_LOCAL_DEBUG("Gracefully shutting down Sink: " + std::to_string((unsigned long) this) + "... Done");
}

bool Sink::finished() {
    return (this->input_buffer->is_query_answers_empty() &&
            this->input_buffer->is_query_answers_finished());
}
