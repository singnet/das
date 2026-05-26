#include "BusCommandRouterProcessor.h"

#include <thread>

#include "BaseQueryProxy.h"
#include "EvolutionMettaParser.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBusSingleton.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace command_router;
using namespace query_engine;
using namespace evolution;
using namespace service_bus;

namespace {

const string CONTEXT_KEY = "context";

}  // namespace

BusCommandRouterProcessor::BusCommandRouterProcessor(shared_ptr<ServiceBus> service_bus)
    : BusCommandProcessor({ServiceBus::BUS_COMMAND_ROUTER}), service_bus(service_bus) {}

BusCommandRouterProcessor::~BusCommandRouterProcessor() {}

shared_ptr<BusCommandProxy> BusCommandRouterProcessor::factory_empty_proxy() {
    return make_shared<BusCommandRouterProxy>();
}

Properties& BusCommandRouterProcessor::parameters_for_peer(const string& peer_id) {
    lock_guard<mutex> lock(this->router_parameters_mutex);
    auto iterator = this->router_parameters_by_peer.find(peer_id);
    if (iterator == this->router_parameters_by_peer.end()) {
        Properties defaults;
        BusCommandRouterProxy::apply_default_parameters(defaults);
        iterator = this->router_parameters_by_peer.emplace(peer_id, defaults).first;
    }
    return iterator->second;
}

void BusCommandRouterProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    auto router_proxy = dynamic_pointer_cast<BusCommandRouterProxy>(proxy);
    if (router_proxy == nullptr) {
        proxy->raise_error_on_peer("Invalid proxy type for BUS_COMMAND_ROUTER");
        return;
    }
    router_proxy->parameters = parameters_for_peer(router_proxy->get_requestor_id());
    try {
        if (router_proxy->get_args().size() < 2) {
            RAISE_ERROR("Invalid bus_command_router args: expected {COMMAND, ARG}");
        }
        const string& command = router_proxy->get_args()[0];
        const string& arg = router_proxy->get_args()[1];
        if (command == "get") {
            handle_get(router_proxy, arg);
        } else if (command == "set") {
            handle_set(router_proxy, arg);
        } else if (command == "query") {
            handle_query(router_proxy, arg);
        } else if (command == "evolution") {
            handle_evolution(router_proxy, arg);
        } else {
            RAISE_ERROR("Unknown router command: " + command);
        }
    } catch (const std::exception& exception) {
        router_proxy->raise_error_on_peer(exception.what());
    }
}

void BusCommandRouterProcessor::handle_get(shared_ptr<BusCommandRouterProxy> proxy, const string& arg) {
    if (arg != "params") {
        RAISE_ERROR("Unsupported get command ARG: " + arg + " (expected 'params')");
    }
    vector<string> response_args;
    vector<string> tokens = proxy->parameters.tokenize();
    for (unsigned int i = 0; i + 2 < tokens.size(); i += 3) {
        response_args.push_back(tokens[i]);
        response_args.push_back(tokens[i + 2]);
    }
    string response = Utils::join(response_args, '\n');
    proxy->to_remote_peer(BusCommandRouterProxy::PARAMS_RESPONSE, {response});
    proxy->to_remote_peer(BaseProxy::FINISHED, {});
}

void BusCommandRouterProcessor::set_router_param(BusCommandRouterProxy* proxy,
                                                 const string& key,
                                                 const string& value) {
    if (value == "true") {
        proxy->parameters[key] = true;
    } else if (value == "false") {
        proxy->parameters[key] = false;
    } else {
        bool is_int = !value.empty();
        for (char c : value) {
            if (!isdigit(c)) {
                is_int = false;
                break;
            }
        }
        if (is_int) {
            proxy->parameters[key] = (unsigned int) stoi(value);
        } else {
            bool is_double = false;
            try {
                size_t idx = 0;
                stod(value, &idx);
                is_double = (idx == value.size());
            } catch (...) {
                is_double = false;
            }
            if (is_double) {
                proxy->parameters[key] = stod(value);
            } else {
                proxy->parameters[key] = value;
            }
        }
    }
}

void BusCommandRouterProcessor::handle_set(shared_ptr<BusCommandRouterProxy> proxy, const string& arg) {
    if (arg.size() < 6 || arg.substr(0, 6) != "param ") {
        RAISE_ERROR("Invalid set ARG (expected 'param <key> <value>'): " + arg);
    }
    string remainder = arg.substr(6);
    size_t key_end = remainder.find(' ');
    if (key_end == string::npos) {
        RAISE_ERROR("Invalid set ARG (missing key or value): " + arg);
    }
    string key = remainder.substr(0, key_end);
    string value = remainder.substr(key_end + 1);
    Utils::trim(key);
    Utils::trim(value);
    if (key.empty() || value.empty()) {
        RAISE_ERROR("Invalid set ARG (empty key or value): " + arg);
    }
    string canonical_key = canonical_evolution_param_key(key);
    if (!canonical_key.empty()) {
        key = canonical_key;
    }
    set_router_param(proxy.get(), key, value);
    {
        lock_guard<mutex> lock(this->router_parameters_mutex);
        this->router_parameters_by_peer[proxy->get_requestor_id()] = proxy->parameters;
    }
    string ack = "Parameter updated: '" + key + "': " + value;
    proxy->to_remote_peer(BusCommandRouterProxy::SET_PARAM_ACK, {ack});
    proxy->to_remote_peer(BaseProxy::FINISHED, {});
}

void BusCommandRouterProcessor::relay_query_answers_to_client(
    shared_ptr<BusCommandRouterProxy> client_proxy, shared_ptr<BaseQueryProxy> downstream) {
    try {
        shared_ptr<QueryAnswer> answer;
        while (!downstream->finished()) {
            while ((answer = downstream->pop()) != nullptr) {
                client_proxy->push(answer);
            }
            Utils::sleep(100);
        }
        while ((answer = downstream->pop()) != nullptr) {
            client_proxy->push(answer);
        }
        if (downstream->error_flag) {
            client_proxy->raise_error_on_peer(downstream->error_message, downstream->error_code);
            return;
        }
        client_proxy->query_processing_finished();
    } catch (const std::exception& exception) {
        LOG_ERROR("Relay to client failed: " << exception.what());
        client_proxy->raise_error_on_peer(exception.what());
    }
}

void BusCommandRouterProcessor::handle_query(shared_ptr<BusCommandRouterProxy> proxy,
                                             const string& arg) {
    string context = proxy->parameters.get<string>(CONTEXT_KEY);
    vector<string> query_tokens = {normalize_metta_percent_variables(arg)};

    auto pm_proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    pm_proxy->parameters = proxy->parameters;
    pm_proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = true;

    shared_ptr<ServiceBus> bus =
        this->service_bus ? this->service_bus : ServiceBusSingleton::get_instance();
    bus->issue_bus_command(pm_proxy);

    proxy->to_remote_peer(BusCommandRouterProxy::ROUTED, {});

    thread relay_thread(
        [client_proxy = proxy, pm_proxy]() { relay_query_answers_to_client(client_proxy, pm_proxy); });
    relay_thread.detach();
}

void BusCommandRouterProcessor::handle_evolution(shared_ptr<BusCommandRouterProxy> proxy,
                                                 const string& arg) {
    string context = proxy->parameters.get<string>(CONTEXT_KEY);

    EvolutionMettaArgs metta_args;
    if (!try_parse_evolution_metta_arg(arg, metta_args) || metta_args.query.empty()) {
        RAISE_ERROR("Evolution ARG must be a labeled MeTTa form starting with (query ...)");
    }

    vector<string> query = {normalize_metta_percent_variables(metta_args.query)};

    string fitness_tag = metta_args.fitness_function_tag;
    if (fitness_tag.empty()) {
        RAISE_ERROR("Missing fitness function tag in Evolution ARG: " + arg);
    }

    auto correlation_queries = metta_correlation_queries(metta_args.correlation_query_expressions);
    auto correlation_replacements =
        metta_correlation_replacements(metta_args.correlation_replacement_groups);
    auto correlation_mappings = metta_correlation_mappings(metta_args.correlation_mapping_groups);

    auto evo_proxy = make_shared<QueryEvolutionProxy>(query,
                                                      correlation_queries,
                                                      correlation_replacements,
                                                      correlation_mappings,
                                                      context,
                                                      fitness_tag);
    evo_proxy->parameters = proxy->parameters;

    shared_ptr<ServiceBus> bus =
        this->service_bus ? this->service_bus : ServiceBusSingleton::get_instance();
    bus->issue_bus_command(evo_proxy);

    proxy->to_remote_peer(BusCommandRouterProxy::ROUTED, {});

    thread relay_thread(
        [client_proxy = proxy, evo_proxy]() { relay_query_answers_to_client(client_proxy, evo_proxy); });
    relay_thread.detach();
}
