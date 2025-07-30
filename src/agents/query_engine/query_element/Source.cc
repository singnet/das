#include "Source.h"

using namespace query_element;

string Source::DEFAULT_ATTENTION_BROKER_PORT = "37007";

// ------------------------------------------------------------------------------------------------
// Constructors and destructors

Source::Source(const string& attention_broker_address) {
    this->attention_broker_address = attention_broker_address;
    this->output_buffer = nullptr;
}

Source::Source() : Source(Source::get_attention_broker_address()) {}

Source::~Source() { 
    this->graceful_shutdown(); 
}

// ------------------------------------------------------------------------------------------------
// Public methods

string Source::get_attention_broker_address() {
    string attention_broker_address = Utils::get_environment("DAS_ATTENTION_BROKER_ADDRESS");
    string attention_broker_port = Utils::get_environment("DAS_ATTENTION_BROKER_PORT");
    if (attention_broker_address.empty()) {
        attention_broker_address = "localhost";
    }
    if (attention_broker_port.empty()) {
        attention_broker_address += ":" + DEFAULT_ATTENTION_BROKER_PORT;
    } else {
        attention_broker_address += ":" + attention_broker_port;
    }
    return attention_broker_address;
}

void Source::setup_buffers() {
    if (this->subsequent_id == "") {
        Utils::error("Invalid empty parent id");
    }
    if (this->id == "") {
        Utils::error("Invalid empty id");
    }
    this->output_buffer = make_shared<QueryNodeClient>(this->id, this->subsequent_id);
}

void Source::graceful_shutdown() {

    if (is_flow_finished()) {
        return;
    }
    if (this->output_buffer != nullptr) {
        this->output_buffer->graceful_shutdown();
    }
}
