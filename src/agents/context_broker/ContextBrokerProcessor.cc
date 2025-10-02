#include "ContextBrokerProcessor.h"

#include <filesystem>

#include "AttentionBrokerClient.h"
#include "ContextBrokerProxy.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace context_broker;
using namespace query_engine;
using namespace service_bus;
using namespace attention_broker;

namespace fs = std::filesystem;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

ContextBrokerProcessor::ContextBrokerProcessor() : BusCommandProcessor({ServiceBus::CONTEXT}) {
    if (AttentionBrokerClient::health_check(true)) {
        LOG_INFO("Connected to AttentionBroker at " + AttentionBrokerClient::SERVER_ADDRESS);
    }
}

ContextBrokerProcessor::~ContextBrokerProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> ContextBrokerProcessor::factory_empty_proxy() {
    shared_ptr<ContextBrokerProxy> proxy(new ContextBrokerProxy());
    return proxy;
}

void ContextBrokerProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    auto context_broker_proxy = dynamic_pointer_cast<ContextBrokerProxy>(proxy);
    string thread_id = "thread<" + proxy->my_id() + "_" + std::to_string(proxy->get_serial()) + ">";
    LOG_DEBUG("Starting new thread: " << thread_id << " to run command: <" << proxy->get_command()
                                      << ">");
    if (this->query_threads.find(thread_id) != this->query_threads.end()) {
        Utils::error("Invalid thread id: " + thread_id);
    } else {
        shared_ptr<StoppableThread> stoppable_thread = make_shared<StoppableThread>(thread_id);
        stoppable_thread->attach(new thread(&ContextBrokerProcessor::thread_process_one_query,
                                            this,
                                            stoppable_thread,
                                            context_broker_proxy));
        this->query_threads[thread_id] = stoppable_thread;
    }
}

// -------------------------------------------------------------------------------------------------
// Private methods

void ContextBrokerProcessor::thread_process_one_query(shared_ptr<StoppableThread> monitor,
                                                      shared_ptr<ContextBrokerProxy> proxy) {
    try {
        if (proxy->args.size() < 2) {
            Utils::error("Syntax error in query command. Missing implicit parameters.");
        }

        proxy->untokenize(proxy->args);

        // Do not allow use_cache == false and enforce_cache_recreation == true
        if (!proxy->get_use_cache() && proxy->get_enforce_cache_recreation()) {
            Utils::error("use_cache == false and enforce_cache_recreation == true is not allowed");
        }

        string command = proxy->get_command();
        if (command == ServiceBus::CONTEXT) {
            LOG_DEBUG("CONTEXT_BROKER proxy: " << proxy->to_string());

            this->set_attention_broker_parameters(proxy->get_initial_rent_rate(),
                                                  proxy->get_initial_spreading_rate_lowerbound(),
                                                  proxy->get_initial_spreading_rate_upperbound());

            this->create_context(monitor, proxy);
        } else {
            Utils::error("Invalid command " + command + " in ContextBrokerProcessor");
        }
    } catch (const std::runtime_error& exception) {
        proxy->raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        proxy->raise_error_on_peer(exception.what());
    }

    // keep alive till client kills it

    LOG_DEBUG("Command finished: <" << proxy->get_command() << ">");
    // TODO add a call to remove_query_thread(monitor->get_id());
}

void ContextBrokerProcessor::create_context(shared_ptr<StoppableThread> monitor,
                                            shared_ptr<ContextBrokerProxy> proxy) {
    LOG_INFO("Processing CONTEXT command for: " << proxy->get_name());

    bool new_cache_flag = false;

    // Check if we can use cached context
    if (!this->read_cache(proxy)) {
        LOG_INFO("Context not cached, processing query...");
        auto pattern_proxy =
            make_shared<PatternMatchingQueryProxy>(proxy->get_query_tokens(), proxy->get_key());
        pattern_proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] =
            proxy->parameters.get_or(BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG, true);
        pattern_proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] =
            proxy->parameters.get_or(BaseQueryProxy::ATTENTION_UPDATE_FLAG, false);
        pattern_proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] =
            proxy->parameters.get_or(BaseQueryProxy::USE_LINK_TEMPLATE_CACHE, false);
        pattern_proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] =
            proxy->parameters.get_or(PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG, false);
        pattern_proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] =
            proxy->parameters.get_or(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS, false);

        ServiceBusSingleton::get_instance()->issue_bus_command(pattern_proxy);

#if LOG_LEVEL >= DEBUG_LEVEL
        unsigned int count_query_answer = 0;
#endif
        // Process query results
        while (!monitor->stopped() && !pattern_proxy->finished()) {
            shared_ptr<QueryAnswer> answer = pattern_proxy->pop();
            if (answer != NULL) {
                for (auto pair : proxy->get_determiner_schema()) {
                    string source = answer->get(pair.first);
                    string target = answer->get(pair.second);
                    if ((source != "") && (target != "")) {
                        proxy->determiner_request.push_back({source, target});
                    }
                }
                for (auto element : proxy->get_stimulus_schema()) {
                    string stimulus = answer->get(element);
                    if (stimulus != "") {
                        proxy->to_stimulate[stimulus] = 1;
                    }
                }
#if LOG_LEVEL >= DEBUG_LEVEL
                if (!(++count_query_answer % 100000)) {
                    LOG_DEBUG("Number of QueryAnswer objects processed so far: " +
                              std::to_string(count_query_answer));
                }
#endif
            } else {
                Utils::sleep();
            }
        }

        if (proxy->get_use_cache()) {
            new_cache_flag = true;
        }

        LOG_INFO("Context processing completed");
    } else {
        LOG_INFO("Context already cached and loaded...");
    }

    if (new_cache_flag || proxy->get_enforce_cache_recreation()) {
        this->write_cache(proxy);
    }

    this->update_attention_broker(proxy);

    LOG_INFO("Context created: name<" << proxy->get_name() << "> with key<" << proxy->get_key() << ">");
    Utils::sleep(1000);

    // Notify caller that context has been created
    proxy->to_remote_peer(ContextBrokerProxy::CONTEXT_CREATED, {});

    // Keep alive till peer (client) kills it
    while (proxy->running() || proxy->update_attention_broker_parameters) {
        if (proxy->update_attention_broker_parameters) {
            this->set_attention_broker_parameters(
                proxy->rent_rate, proxy->spreading_rate_lowerbound, proxy->spreading_rate_upperbound);
            proxy->update_attention_broker_parameters = false;
            proxy->to_remote_peer(ContextBrokerProxy::ATTENTION_BROKER_SET_PARAMETERS_FINISHED, {});
            break;
        }
        Utils::sleep();
    }
}

void ContextBrokerProcessor::set_attention_broker_parameters(double rent_rate,
                                                             double spreading_rate_lowerbound,
                                                             double spreading_rate_upperbound) {
    LOG_INFO("Setting AttentionBroker parameters { rent_rate: "
             << rent_rate << ", spreading_rate_lowerbound: " << spreading_rate_lowerbound
             << ", spreading_rate_upperbound: " << spreading_rate_upperbound << " }");
    AttentionBrokerClient::set_parameters(
        rent_rate, spreading_rate_lowerbound, spreading_rate_upperbound);
}

static inline void read_line(ifstream& file, string& line) {
    if (!getline(file, line)) {
        Utils::error("Error reading a line from cache file");
    }
}

void ContextBrokerProcessor::update_attention_broker(shared_ptr<ContextBrokerProxy> proxy) {
    auto key = proxy->get_key();
    LOG_INFO("Context(key<" << key << ">) updating AttentionBroker");

    AttentionBrokerClient::set_determiners(proxy->determiner_request, key);
    if (proxy->to_stimulate.size() > 0) {
        AttentionBrokerClient::stimulate(proxy->to_stimulate, key);
    }
    proxy->to_stimulate.clear();
    proxy->determiner_request.clear();
}

void ContextBrokerProcessor::write_cache(shared_ptr<ContextBrokerProxy> proxy) {
    // Delete cache file if it exists
    auto cache_file_name = proxy->get_cache_file_name();
    if (fs::exists(cache_file_name)) {
        fs::remove(cache_file_name);
    }

    LOG_INFO("Caching computed context into file: " + cache_file_name);

    ofstream file(cache_file_name);

    if (!file.is_open()) {
        LOG_ERROR("Couldn't open file " + cache_file_name + " for writing");
        LOG_INFO("Context info wont be cached");
        return;
    }

    file << proxy->to_stimulate.size() << endl;
    for (auto pair : proxy->to_stimulate) {
        file << pair.first << endl;
    }
    file << proxy->determiner_request.size() << endl;
    for (auto sub_vector : proxy->determiner_request) {
        file << sub_vector.size() << endl;
        for (string handle : sub_vector) {
            file << handle << endl;
        }
    }

    file.close();
}

bool ContextBrokerProcessor::read_cache(shared_ptr<ContextBrokerProxy> proxy) {
    if (!proxy->get_use_cache()) {
        LOG_INFO("No cached info will be used (use_cache is false)");
        return false;
    }

    auto cache_file_name = proxy->get_cache_file_name();

    ifstream file(cache_file_name);
    if (file.is_open()) {
        LOG_INFO("Reading Context info from cache file: " + cache_file_name);
    } else {
        LOG_INFO("Couldn't open file " + cache_file_name + " for reading");
        LOG_INFO("No cached info will be used (file not found)");
        return false;
    }

    string line;
    read_line(file, line);
    int size = Utils::string_to_int(line);
    for (int i = 0; i < size; i++) {
        read_line(file, line);
        proxy->to_stimulate[line] = 1;
    }
    read_line(file, line);
    size = Utils::string_to_int(line);
    proxy->determiner_request.reserve(size);
    vector<string> sub_vector;
    for (int i = 0; i < size; i++) {
        read_line(file, line);
        int sub_vector_size = Utils::string_to_int(line);
        sub_vector.clear();
        sub_vector.reserve(sub_vector_size);
        for (int j = 0; j < sub_vector_size; j++) {
            read_line(file, line);
            sub_vector.push_back(line);
        }
        proxy->determiner_request.push_back(sub_vector);
    }

    file.close();
    return true;
}
