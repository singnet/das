#include "LinkCreationProcessor.h"

#if defined(__GLIBC__)
#include <malloc.h>
#endif

#include "Logger.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"

// Note to reviewer: left here to help in a follow-up refactor regarding the use of MORK
// instead of Redis+Mongo
#define USE_MORK ((bool) false)

using namespace link_creation_agent;
using namespace query_engine;
using namespace atoms;
using namespace service_bus;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

LinkCreationProcessor::LinkCreationProcessor() : BusCommandProcessor({ServiceBus::LINK_CREATION}) {
    // AttentionBrokerClient::health_check(true);
}

LinkCreationProcessor::~LinkCreationProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> LinkCreationProcessor::factory_empty_proxy() {
    shared_ptr<LinkCreationProxy> proxy(new LinkCreationProxy());
    return proxy;
}

void LinkCreationProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->thread_management_mutex);
    auto link_creation_proxy = dynamic_pointer_cast<LinkCreationProxy>(proxy);
    if (link_creation_proxy == nullptr) {
        RAISE_ERROR("Invalid BusCommandProxy instance");
    }
    string thread_id = "thread<" + proxy->my_id() + "_" + std::to_string(proxy->get_serial()) + ">";
    LOG_DEBUG("Starting new thread: " << thread_id << " to run command: <" << proxy->get_command()
                                      << ">");
    if (this->processor_threads.find(thread_id) != this->processor_threads.end()) {
        RAISE_ERROR("Invalid thread id: " + thread_id);
    } else {
        shared_ptr<StoppableThread> stoppable_thread = make_shared<StoppableThread>(thread_id);
        stoppable_thread->attach(new thread(&LinkCreationProcessor::thread_process_one_query,
                                            this,
                                            stoppable_thread,
                                            link_creation_proxy));
        this->processor_threads[thread_id] = stoppable_thread;
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
    proxy->query_processing_finished();
    // Self-reap: detach this finished thread and drop it from processor_threads immediately, so a
    // burst that finishes while the node is idle returns to baseline without waiting for the next
    // command. A thread cannot join itself, hence detach instead of join.
    {
        lock_guard<mutex> semaphore(this->thread_management_mutex);
        this->processor_threads.erase(monitor->get_id());
        monitor->detach();
    }
    LOG_DEBUG("Command finished: <" << proxy->get_command() << ">");
}

shared_ptr<PatternMatchingQueryProxy> LinkCreationProcessor::issue_link_creation_query(
    shared_ptr<LinkCreationProxy> proxy) {
    auto pm_proxy =
        make_shared<PatternMatchingQueryProxy>(proxy->get_query_tokens(), proxy->get_context());
    pm_proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] =
        (unsigned int) proxy->parameters.get<unsigned int>(BaseQueryProxy::ATTENTION_CORRELATION);
    pm_proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] =
        (unsigned int) proxy->parameters.get<unsigned int>(BaseQueryProxy::ATTENTION_UPDATE);
    pm_proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] =
        (bool) proxy->parameters.get<bool>(BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG);
    pm_proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] =
        (bool) proxy->parameters.get<bool>(BaseQueryProxy::USE_LINK_TEMPLATE_CACHE);
    pm_proxy->parameters[BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH] =
        (bool) proxy->parameters.get<bool>(BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH);
    pm_proxy->parameters[BaseQueryProxy::ATTENTION_FOCUS_STRICTNESS] =
        (double) proxy->parameters.get<double>(LinkCreationProxy::ATTENTION_FOCUS_STRICTNESS);
    pm_proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] =
        (bool) proxy->parameters.get<bool>(PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG);
    pm_proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] =
        (bool) proxy->parameters.get<bool>(PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG);
    pm_proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] =
        (bool) proxy->parameters.get<bool>(PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG);
    pm_proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) 0;
    pm_proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] =
        (proxy->get_query_tokens().size() == 1);
    pm_proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = true;

    ServiceBusSingleton::get_instance()->issue_bus_command(pm_proxy);
    return pm_proxy;
}

void LinkCreationProcessor::remove_processor_thread(const string& stoppable_thread_id) {
    lock_guard<mutex> semaphore(this->thread_management_mutex);
    auto iterator = this->processor_threads.find(stoppable_thread_id);
    if (iterator == this->processor_threads.end()) {
        RAISE_ERROR("Attempt to remove a StoppableThread that doesn't exist: " + stoppable_thread_id);
    }
    this->processor_threads.erase(iterator);
}

bool LinkCreationProcessor::limit_reached(shared_ptr<LinkCreationProxy> proxy,
                                          unsigned int count,
                                          string& property) {
    bool answer = false;
    unsigned int property_value = proxy->parameters.get<unsigned int>(property);
    if (property_value > 0) {
        answer = (count >= property_value);
    }
    return answer;
}

void LinkCreationProcessor::link_creation(shared_ptr<StoppableThread> monitor,
                                          shared_ptr<LinkCreationProxy> proxy) {
    STACK_TRACE();
    unsigned int count_created = 0;
    unsigned int count_used_query_answers = 0;
    unsigned int count_iterated_query_answers = 0;
    unsigned int count_query_answers_in_cycle = 0;
    unsigned int unproductive_visit = 0;
    unsigned int visit_attempts = 0;
    shared_ptr<QueryAnswer> query_answer;
    LinkCreationStats stats;
    while (!(monitor->stopped() || proxy->stop_criteria_met())) {
        while (!monitor->stopped() && !proxy->is_cycle_start_allowed()) {
            Utils::sleep();
        }
        if (monitor->stopped()) {
            break;
        }
        // Starting one round of link creation
        auto pm_proxy = issue_link_creation_query(proxy);
        count_query_answers_in_cycle = 0;
        visit_attempts = 0;
        unproductive_visit = 0;
        while (!(monitor->stopped())) {
            // Draining query results
            if ((query_answer = pm_proxy->pop()) == nullptr) {
                if (pm_proxy->finished()) {
                    break;
                }
                Utils::sleep();
            } else {
                count_iterated_query_answers++;
                LOG_DEBUG("Iterating query answer " + to_string(count_iterated_query_answers) + ": " +
                          query_answer->to_string(USE_MORK));
                stats = proxy->link_creation(query_answer);
                if (stats.created > 0) {
                    count_created += stats.created;
                    count_used_query_answers++;
                    count_query_answers_in_cycle++;
                    LOG_DEBUG("Created links: " + std::to_string(count_query_answers_in_cycle));
                    unproductive_visit = 0;
                    visit_attempts = 0;
                    proxy->push(query_answer);
                    if (limit_reached(proxy,
                                      count_query_answers_in_cycle,
                                      LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND)) {
                        break;
                    }
                } else if (stats.visited) {
                    unproductive_visit++;
                    LOG_DEBUG("Unproductive visit: " + std::to_string(unproductive_visit));
                    visit_attempts = 0;
                    if (limit_reached(proxy,
                                      unproductive_visit,
                                      LinkCreationProxy::MAX_UNPRODUCTIVE_VISITS_PER_ROUND)) {
                        break;
                    }
                } else {
                    visit_attempts++;
                    if (limit_reached(
                            proxy, visit_attempts, LinkCreationProxy::MAX_VISIT_ATTEMPTS_PER_ROUND)) {
                        break;
                    }
                }
            }
        }
        proxy->flush_determiners();
        proxy->flush_answer_bundle();
        proxy->cycle_ended();
        if (!pm_proxy->finished()) {
            // stopping pattern matching query
            pm_proxy->abort();
        }
        proxy->inc_round_count();
        LOG_DEBUG("Cycle ended");
    }
    LOG_INFO("Iterated through a total of " + to_string(count_iterated_query_answers) +
             " query answers");
    LOG_INFO("Used a total of " + to_string(count_used_query_answers) +
             " query answers to create new links");
    LOG_INFO("Built a total of " + to_string(count_created) + " links");
}
