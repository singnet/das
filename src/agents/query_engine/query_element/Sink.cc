#include "Sink.h"

#define LOG_LEVEL LEVEL_DEBUG
#include "Logger.h"

using namespace query_element;

// ------------------------------------------------------------------------------------------------
// Constructors and destructors

Sink::Sink(shared_ptr<QueryElement> precedent, const string& id, bool setup_buffers_flag) {
    this->precedent = precedent;
    this->id = id;
    if (setup_buffers_flag) {
        setup_buffers();
    }
}

Sink::~Sink() { 
    this->stop();
}

// ------------------------------------------------------------------------------------------------
// Public methods

void Sink::setup_buffers() {
    if (this->subsequent_id != "") {
        Utils::error("Invalid non-empty subsequent id: " + this->subsequent_id);
    }
    if (this->id == "") {
        Utils::error("Invalid empty id");
    }
    this->input_buffer = make_shared<QueryNodeServer>(this->id);
    this->precedent->subsequent_id = this->id;
    this->precedent->setup_buffers();
}

void Sink::stop() {
    if (! check_and_set_stopped()) {
        LOG_DEBUG("Stopping SINK: " << this->id);
        this->input_buffer->stop();
        this->precedent->stop();
    }
}

bool Sink::finished() {
    return (this->input_buffer->is_query_answers_empty() &&
            this->input_buffer->is_query_answers_finished());
}
