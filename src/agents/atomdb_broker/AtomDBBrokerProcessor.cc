#include "AtomDBBrokerProcessor.h"

#include "ServiceBus.h"
#include "ServiceBusSingleton.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb_broker;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

AtomDBBrokerProcessor::AtomDBBrokerProcessor() : BusCommandProcessor({ServiceBus::ATOMDB}) {}

AtomDBBrokerProcessor::~AtomDBBrokerProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> AtomDBBrokerProcessor::factory_empty_proxy() {
    return make_shared<AtomDBBrokerProxy>();
}

void AtomDBBrokerProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    auto atomdb_broker_proxy = dynamic_pointer_cast<AtomDBBrokerProxy>(proxy);
    string thread_id = "thread<" + proxy->my_id() + "_" + std::to_string(proxy->get_serial()) + ">";
    LOG_DEBUG("Starting new thread: " << thread_id << " to run command: <" << proxy->get_command() << ">");
    if (this->query_threads.find(thread_id) != this->query_threads.end()) {
        Utils::error("Invalid thread id: " + thread_id);
    } else {
        shared_ptr<StoppableThread> stoppable_thread = make_shared<StoppableThread>(thread_id);
        stoppable_thread->attach(new thread(&AtomDBBrokerProcessor::thread_process_one_query, this, stoppable_thread, atomdb_broker_proxy));
        this->query_threads[thread_id] = stoppable_thread;
    }
}

void AtomDBBrokerProcessor::thread_process_one_query(shared_ptr<StoppableThread> monitor, shared_ptr<AtomDBBrokerProxy> proxy) {
    try {
        if (proxy->args.size() < 2) {
            Utils::error("Syntax error in query command. Missing implicit parameters.");
        }

        proxy->untokenize(proxy->args);

        string command = proxy->get_command();
        string action = proxy->get_action();

        if (command == ServiceBus::ATOMDB && action == AtomDBBrokerProxy::ADD_ATOMS) {
            LOG_DEBUG("AtomDB Broker proxy: " << proxy->to_string());
            proxy->to_remote_peer(AtomDBBrokerProxy::ADD_ATOMS, {});
        } else {
            Utils::error("Invalid command " + command + " in AtomDBBrokerProcessor");
        }
    } catch (const std::runtime_error& exception) {
        proxy->raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        proxy->raise_error_on_peer(exception.what());
    }

    LOG_DEBUG("Command finished: <" << proxy->get_command() << ">");

}
