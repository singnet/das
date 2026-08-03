#include "LinkCreationProcessor.h"

#if defined(__GLIBC__)
#include <malloc.h>
#endif

#include "Logger.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"

using namespace link_creation_agent;
using namespace query_engine;
using namespace atoms;
using namespace service_bus;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

LinkCreationProcessor::LinkCreationProcessor() : BusCommandProcessor({ServiceBus::LINK_CREATION}) {
    //AttentionBrokerClient::health_check(true);
}

LinkCreationProcessor::~LinkCreationProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> LinkCreationProcessor::factory_empty_proxy() {
    shared_ptr<LinkCreationProxy> proxy(new LinkCreationProxy());
    return proxy;
}

void LinkCreationProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    auto link_creation_proxy = dynamic_pointer_cast<LinkCreationProxy>(proxy);
    string thread_id = "thread<" + proxy->my_id() + "_" + std::to_string(proxy->get_serial()) + ">";
    LOG_DEBUG("Starting new thread: " << thread_id << " to run command: <" << proxy->get_command()
                                      << ">");
    if (this->query_threads.find(thread_id) != this->query_threads.end()) {
        RAISE_ERROR("Invalid thread id: " + thread_id);
    } else {
        shared_ptr<StoppableThread> stoppable_thread = make_shared<StoppableThread>(thread_id);
        stoppable_thread->attach(new thread(
            &LinkCreationProcessor::thread_process_one_query, this, stoppable_thread, link_creation_proxy));
        this->query_threads[thread_id] = stoppable_thread;
    }
}

// -------------------------------------------------------------------------------------------------
// Private methods

void LinkCreationProcessor::thread_process_one_query(shared_ptr<StoppableThread> monitor,
                                                       shared_ptr<LinkCreationProxy> proxy) {
    try {
        if (proxy->args.size() < 2) {
            RAISE_ERROR("Syntax error in query command. Missing implicit parameters.");
        }
        proxy->untokenize(proxy->args);
        string command = proxy->get_command();
        if (command == ServiceBus::LINK_CREATION) {
            LOG_INFO("Proxy: " << proxy->to_string());
            this->link_creation(monitor, proxy);
        } else {
            RAISE_ERROR("Invalid command " + command + " in LinkCreationProcessor");
        }
    } catch (const std::runtime_error& exception) {
        proxy->raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        proxy->raise_error_on_peer(exception.what());
    }
#if defined(__GLIBC__)
    // Release freed heap to the OS
    malloc_trim(0);
#endif
    // Self-reap: detach this finished thread and drop it from query_threads immediately, so a
    // burst that finishes while the node is idle returns to baseline without waiting for the next
    // command. A thread cannot join itself, hence detach instead of join.
    {
        lock_guard<mutex> semaphore(this->query_threads_mutex);
        this->query_threads.erase(monitor->get_id());
        monitor->detach();
    }
    LOG_DEBUG("Command finished: <" << proxy->get_command() << ">");
}

shared_ptr<PatternMatchingQueryProxy> LinkCreationProcessor::issue_link_creation_query(
    shared_ptr<LinkCreationProxy> proxy) {
    // TBD
    return nullptr;
}

void LinkCreationProcessor::remove_query_thread(const string& stoppable_thread_id) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    this->query_threads.erase(this->query_threads.find(stoppable_thread_id));
}

void LinkCreationProcessor::link_creation(shared_ptr<StoppableThread> monitor,
                                           shared_ptr<LinkCreationProxy> proxy) {
    // TBD
}

