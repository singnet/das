#include "QueryEvolutionProcessor.h"

#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace evolution;
using namespace query_engine;

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
        proxy->untokenize(proxy->args);
        LOG_DEBUG("proxy: " + proxy->to_string());
        string command = proxy->get_command();
        if (command == ServiceBus::QUERY_EVOLUTION) {
            LOG_DEBUG("QUERY_EVOLUTION proxy: " << proxy->to_string());
            this->evolve_query(monitor, proxy);
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

void QueryEvolutionProcessor::sample_population(
    shared_ptr<StoppableThread> monitor,
    shared_ptr<QueryEvolutionProxy> proxy,
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& population) {
    unsigned int population_size = proxy->parameters.get<unsigned int>(QueryEvolutionProxy::POPULATION_SIZE);
    auto pm = atom_space.pattern_matching_query(
        proxy->get_query_tokens(), population_size, proxy->get_context());
    while ((!pm->finished()) && (!monitor->stopped()) && (population.size() < population_size)) {
        shared_ptr<QueryAnswer> answer = pm->pop();
        if (answer != NULL) {
            float fitness = proxy->compute_fitness(answer);
            population.push_back(make_pair(answer, fitness));
        } else {
            Utils::sleep();
        }
    }
    if (!pm->finished()) {
        // TODO Uncomment this when abort() is implemented
        // pm->abort();
        LOG_ERROR("Couldn't abort pattern matching query because abort() is not implemented");
        unsigned int count = 0;
        while (!pm->finished()) {
            shared_ptr<QueryAnswer> query_answer;
            if ((query_answer = pm->pop()) == NULL) {
                Utils::sleep();
            } else {
                count++;
            }
        }
        LOG_ERROR("Discarding " << count << " answers");
    }
}

void QueryEvolutionProcessor::evolve_query(shared_ptr<StoppableThread> monitor,
                                           shared_ptr<QueryEvolutionProxy> proxy) {
    vector<std::pair<shared_ptr<QueryAnswer>, float>> population;
    while (!monitor->stopped()) {
        sample_population(monitor, proxy, population);
        LOG_DEBUG("New population sampled: " << population.size() << " individuals.");
        break;
    }
}

void QueryEvolutionProcessor::remove_query_thread(const string& stoppable_thread_id) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    this->query_threads.erase(this->query_threads.find(stoppable_thread_id));
}
