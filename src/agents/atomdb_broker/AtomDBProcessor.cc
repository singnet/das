#include "AtomDBProcessor.h"

#include "AtomDBProxy.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "StoppableThread.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb_broker;

// -------------------------------------------------------------------------------------------------
// Constructor and destructor

AtomDBProcessor::AtomDBProcessor() : BusCommandProcessor({ServiceBus::ATOMDB}) {}

AtomDBProcessor::~AtomDBProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> AtomDBProcessor::factory_empty_proxy() { return make_shared<AtomDBProxy>(); }

void AtomDBProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    auto atomdb_proxy = dynamic_pointer_cast<AtomDBProxy>(proxy);
    string thread_id = "thread<" + proxy->my_id() + "_" + std::to_string(proxy->get_serial()) + ">";
    LOG_DEBUG("Starting new thread: " << thread_id << " to run command: <" << proxy->get_command()
                                      << ">");
    if (this->query_threads.find(thread_id) != this->query_threads.end()) {
        Utils::error("Invalid thread id: " + thread_id);
    } else {
        shared_ptr<StoppableThread> stoppable_thread = make_shared<StoppableThread>(thread_id);
        stoppable_thread->attach(new thread(
            &AtomDBProcessor::thread_process_one_query, this, stoppable_thread, atomdb_proxy));
        this->query_threads[thread_id] = stoppable_thread;
    }
}

void AtomDBProcessor::thread_process_one_query(shared_ptr<StoppableThread> monitor,
                                               shared_ptr<AtomDBProxy> proxy) {
    try {
        proxy->untokenize(proxy->args);
        string command = proxy->get_command();
        if (command == ServiceBus::ATOMDB) {
            while (!proxy->is_aborting()) {
                Utils::sleep();
            }
        } else {
            Utils::error("Invalid command " + command + " in AtomDBProcessor");
        }
    } catch (const std::runtime_error& exception) {
        proxy->raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        proxy->raise_error_on_peer(exception.what());
    }
    LOG_DEBUG("Command finished: <" << command << ">");
}