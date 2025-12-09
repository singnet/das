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
    this->parent_processor = nullptr;
}

Processor::~Processor() {
    if (this->current_state != FINISHED) {
        // This doesn't throw an exception
        Utils::error("Destroying processor " + this->id + " without explicitly stopping it.", false);
    }
}

void Processor::bind_subprocessor(shared_ptr<Processor> root, shared_ptr<Processor> child) {
    // static method
    if (root->is_setup() || child->is_setup()) {
        Utils::error("Can't bind processors after setup() has been called");
    }
    if (child->parent_processor != nullptr) {
        Utils::error("Invalid attempt to bind processor " + child->to_string() + " to " +
                     root->to_string() + ". It's already bound to " +
                     child->parent_processor->to_string());
    }
    root->subprocessors.push_back(child);
    child->parent_processor = root;
    root->add_subprocessor(child);
    child->set_parent(root);
}

void Processor::setup() {
    this->api_mutex.lock();
    check_state("setup", WAITING_SETUP);
    this->current_state = WAITING_START;
    this->api_mutex.unlock();
    for (auto subprocess : this->subprocessors) {
        if (!subprocess->is_setup()) {
            subprocess->setup();
        }
    }
    if ((this->parent_processor != nullptr) && !this->parent_processor->is_setup()) {
        this->parent_processor->setup();
    }
}

void Processor::start() {
    this->api_mutex.lock();
    check_state("start", WAITING_START);
    this->current_state = WAITING_STOP;
    this->api_mutex.unlock();
    for (auto subprocess : this->subprocessors) {
        if (!subprocess->is_running()) {
            subprocess->start();
        }
    }
    if ((this->parent_processor != nullptr) && !this->parent_processor->is_running()) {
        this->parent_processor->start();
    }
}

void Processor::stop() {
    this->api_mutex.lock();
    check_state("stop", WAITING_STOP);
    this->current_state = FINISHED;
    this->api_mutex.unlock();
    for (auto subprocess : this->subprocessors) {
        if (!subprocess->is_finished()) {
            subprocess->stop();
        }
    }
    if ((this->parent_processor != nullptr) && !this->parent_processor->is_finished()) {
        this->parent_processor->stop();
    }
    this->subprocessors.clear();
    this->parent_processor = nullptr;
}

bool Processor::is_setup() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->current_state > WAITING_SETUP);
}

bool Processor::is_running() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->current_state == WAITING_STOP);
}

bool Processor::is_finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->current_state == FINISHED);
}

void Processor::add_subprocessor(shared_ptr<Processor> other) {}

void Processor::set_parent(shared_ptr<Processor> other) {}

string Processor::to_string() { return this->id; }

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
        this->api_mutex.unlock();
        Utils::error(error_message);
    }
}
