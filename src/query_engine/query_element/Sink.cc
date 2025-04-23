#include "Sink.h"

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

Sink::~Sink() { this->input_buffer->graceful_shutdown(); }

// ------------------------------------------------------------------------------------------------
// Public methods

void Sink::setup_buffers() {
    if (!this->consumers.empty()) {
        string ids;
        for (auto& consumer : this->consumers) {
            ids += consumer + " ";
        }
        Utils::error("Invalid non-empty subsequent ids: " + ids);
    }
    if (this->id == "") {
        Utils::error("Invalid empty id");
    }
    this->input_buffer = make_shared<QueryNodeServer>(this->id);
    this->precedent->subscribe(this->id);
    this->precedent->setup_buffers();
}

void Sink::graceful_shutdown() {
    this->input_buffer->graceful_shutdown();
    this->precedent->graceful_shutdown();
}

bool Sink::finished() {
    return (this->input_buffer->is_query_answers_empty() &&
            this->input_buffer->is_query_answers_finished());
}
