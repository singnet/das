#include "Source.h"
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace query_element;

// ------------------------------------------------------------------------------------------------
// Constructors and destructors

Source::Source() {
    LOG_LOCAL_DEBUG("Creating Source: " + std::to_string((unsigned long) this));
    this->output_buffer = nullptr;
}

Source::~Source() {
    LOG_LOCAL_DEBUG("Deleting Source: " + std::to_string((unsigned long) this) + "...");
    this->graceful_shutdown();
    LOG_LOCAL_DEBUG("Deleting Source: " + std::to_string((unsigned long) this) + "... Done");
}

// ------------------------------------------------------------------------------------------------
// Public methods

void Source::setup_buffers() {
    LOG_LOCAL_DEBUG("Setting up buffers for Source: " + std::to_string((unsigned long) this) + "...");
    if (this->subsequent_id == "") {
        Utils::error("Invalid empty parent id");
    }
    if (this->id == "") {
        Utils::error("Invalid empty id");
    }
    this->output_buffer = make_shared<QueryNodeClient>(this->id, this->subsequent_id);
    LOG_LOCAL_DEBUG("Setting up buffers for Source: " + std::to_string((unsigned long) this) +
                    "... Done");
}

void Source::graceful_shutdown() {
    LOG_LOCAL_DEBUG("Gracefully shutting down Source: " + std::to_string((unsigned long) this) + "...");
    if (is_flow_finished()) {
        return;
    }
    if (this->output_buffer != nullptr) {
        LOG_LOCAL_DEBUG("Gracefully shutting down output buffer of Source: " +
                        std::to_string((unsigned long) this) + "...");
        this->output_buffer->graceful_shutdown();
        LOG_LOCAL_DEBUG("Gracefully shutting down output buffer of Source: " +
                        std::to_string((unsigned long) this) + "... Done");
    }
    LOG_LOCAL_DEBUG("Gracefully shutting down Source: " + std::to_string((unsigned long) this) +
                    "... Done");
}
