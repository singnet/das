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

Sink::~Sink() {
    this->input_buffer->graceful_shutdown();
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

void Sink::graceful_shutdown() {
    this->input_buffer->graceful_shutdown();
    this->precedent->graceful_shutdown();
}
