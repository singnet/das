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
    LOG_DEBUG("Stopping SINK: " << this->id);
    if (! stopped()) {
        cout << "Precedent: " << this->precedent->id << endl;
        cout << "XXXXXXXXXXXXXXXXXXXXXXX 1" << endl;
        QueryElement::stop();
        cout << "XXXXXXXXXXXXXXXXXXXXXXX 2" << endl;
        this->input_buffer->stop();
        cout << "XXXXXXXXXXXXXXXXXXXXXXX 3" << endl;
        this->precedent->stop();
        cout << "XXXXXXXXXXXXXXXXXXXXXXX 4" << endl;
    } else {
        cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX Sink::stop() Already stopped" << endl;
    }
}

bool Sink::finished() {
    return (this->input_buffer->is_query_answers_empty() &&
            this->input_buffer->is_query_answers_finished());
}
