#include "QueryEvolutionProcessor.h"

#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace evolution;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

QueryEvolutionProcessor::QueryEvolutionProcessor()
    : BusCommandProcessor({ServiceBus::QUERY_EVOLUTION}) {}

QueryEvolutionProcessor::~QueryEvolutionProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> QueryEvolutionProcessor::factory_empty_proxy() {
    shared_ptr<QueryEvolutionProxy> proxy(new QueryEvolutionProxy());
    return proxy;
}

void QueryEvolutionProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    auto query_proxy = dynamic_pointer_cast<QueryEvolutionProxy>(proxy);
    string thread_id = "thread<" + proxy->my_id() + "_" + std::to_string(proxy->get_serial()) + ">";
    LOG_DEBUG("Starting new thread: " << thread_id << " to run command: <" << proxy->get_command()
                                      << ">");
    if (this->query_threads.find(thread_id) != this->query_threads.end()) {
        Utils::error("Invalid thread id: " + thread_id);
    } else {
        shared_ptr<StoppableThread> stoppable_thread = make_shared<StoppableThread>(thread_id);
        stoppable_thread->attach(new thread(
            &QueryEvolutionProcessor::thread_process_one_query, this, stoppable_thread, query_proxy));
        this->query_threads[thread_id] = stoppable_thread;
    }
}

// -------------------------------------------------------------------------------------------------
// Private methods

void QueryEvolutionProcessor::thread_process_one_query(shared_ptr<StoppableThread> monitor,
                                                       shared_ptr<QueryEvolutionProxy> proxy) {
    try {
        if (proxy->args.size() < 2) {
            Utils::error("Syntax error in query command. Missing implicit parameters.");
        }
        unsigned int skip_arg = 0;
        proxy->set_context(proxy->args[skip_arg++]);
        proxy->set_unique_assignment_flag(proxy->args[skip_arg++] == "1");
        proxy->query_tokens.insert(
            proxy->query_tokens.begin(), proxy->args.begin() + skip_arg, proxy->args.end());
        string command = proxy->get_command();
        if (command == ServiceBus::QUERY_EVOLUTION) {
        } else {
            Utils::error("Invalid command " + command + " in QueryEvolutionProcessor");
        }
    } catch (const std::runtime_error& exception) {
        proxy->raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        proxy->raise_error_on_peer(exception.what());
    }
    LOG_DEBUG("Command finished: <" << proxy->get_command() << ">");
    // TODO add a call to remove_query_thread(monitor->get_id());
}

void QueryEvolutionProcessor::remove_query_thread(const string& stoppable_thread_id) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    this->query_threads.erase(this->query_threads.find(stoppable_thread_id));
}
