#include "AtomDBProcessor.h"

#include "AtomDBProxy.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "StoppableThread.h"
#include "Utils.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace atomdb_broker;

// -------------------------------------------------------------------------------------------------
// Constructor and destructor

AtomDBProcessor::AtomDBProcessor() : BusCommandProcessor({ServiceBus::ATOMDB}) {
    // Run manage_finished_threads in a separate thread
    this->stop_flag = false;
    this->thread_management = thread(&AtomDBProcessor::manage_finished_threads, this);
}

AtomDBProcessor::~AtomDBProcessor() {
    LOG_INFO("Shutting down AtomDBProcessor...");
    this->stop_flag = true;
    LOG_INFO("Waiting for thread management to finish...");
    this->thread_management.join();
    LOG_INFO("Stopping all query threads...");
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    for (auto& pair : this->query_threads) {
        LOG_INFO("Stopping thread: " << pair.first);
        pair.second->stop();
    }
}

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
    string command = proxy->get_command();
    try {
        proxy->untokenize(proxy->args);
        if (command == ServiceBus::ATOMDB) {
            while (!proxy->is_aborting() && !this->stop_flag) {
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

    LOG_INFO("Aborting command: <" << command << "> in thread: " << monitor->get_id());
    this->query_threads[monitor->get_id()]->stop(false);
}

void AtomDBProcessor::manage_finished_threads() {
    while (!this->stop_flag) {
        for (auto it = this->query_threads.begin(); it != this->query_threads.end();) {
            if (it->second->stopped()) {
                lock_guard<mutex> semaphore(this->query_threads_mutex);
                LOG_DEBUG("Removing finished thread: " << it->first);
                it->second->stop();
                it = this->query_threads.erase(it);
            } else {
                ++it;
            }
        }
        Utils::sleep(1000);
    }
}
