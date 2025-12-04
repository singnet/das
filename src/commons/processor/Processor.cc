#include "Processor.h"
#include "Utils.h"

using namespace commons;
using namespace processor;

// -------------------------------------------------------------------------------------------------
// Public methods

Processor::Processor(const string& id) {
    if (id == "") {
        Utils::error("Invalid empty id for processor");
    }
    this->current_state = WAITING_SETUP;
    this->id = id;
}

Processor::~Processor() {
    if (this->current_state != FINISHED) {
        // This doesn't throw an exception
        Utils::error("Destroying processor " + this->id + " without explicitly stopping it.", false);
    }
}

void Processor::setup() {
    lock_guard<mutex> semaphore(this->api_mutex);
    check_state("setup", WAITING_SETUP);
    this->current_state = WAITING_START;
}

void Processor::start() {
    lock_guard<mutex> semaphore(this->api_mutex);
    check_state("start", WAITING_START);
    this->current_state = WAITING_STOP;
}

void Processor::stop() {
    lock_guard<mutex> semaphore(this->api_mutex);
    check_state("stop", WAITING_STOP);
    this->current_state = FINISHED;
}

bool Processor::is_setup() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->current_state > WAITING_SETUP);
}

bool Processor::is_finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->current_state == FINISHED);
}

string Processor::to_string() {
    return this->id;
}

// -------------------------------------------------------------------------------------------------
// Private methods

void Processor::check_state(const string& action, State state) {
    if (this->current_state != state) {
        string error_message = "Invalid attempt to " + action + " processor " + this->id + ". ";
        if (this->current_state == WAITING_SETUP) {
            error_message += "Processor is not setup.";
        } else if (this->current_state == WAITING_START) {
            if (action == "setup") {
                error_message += "Processor is already setup.";
            } else {
                error_message += "Processor is not running.";
            }
        } else if (this->current_state == WAITING_STOP) {
            error_message += "Processor is already running.";
        } else if (this->current_state == FINISHED) {
            error_message += "Processor is finished.";
        } else {
            error_message += "Processor is in unexpected state: " + this->current_state;
        }
        Utils::error(error_message);
    }
}
