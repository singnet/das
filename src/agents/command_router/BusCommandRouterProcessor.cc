#include "BusCommandRouterProcessor.h"

#include <algorithm>
#include <atomic>
#include <variant>

#include "BaseQueryProxy.h"
#include "EvolutionMettaParser.h"
#include "PatternMatchingQueryProxy.h"
#include "PortPool.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace command_router;
using namespace query_engine;
using namespace evolution;
using namespace service_bus;

namespace {

const string CONTEXT_KEY = "context";
atomic<unsigned int> HTTP_REQUEST_SERIAL{1};

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

void BusCommandRouterProcessor::dispatch_http_command(
    const shared_ptr<BusCommandRouterProxy>& caller_proxy, const string& http_requestor_id) {
    if (caller_proxy == nullptr) {
        RAISE_ERROR("HTTP command requires a caller proxy");
    }
    if (caller_proxy->issued) {
        RAISE_ERROR("Attempt to dispatch the same HTTP caller proxy twice");
    }
    if (http_requestor_id.empty()) {
        RAISE_ERROR("HTTP command dispatch requires a non-empty http_requestor_id");
    }

    const unsigned int serial = HTTP_REQUEST_SERIAL.fetch_add(1);

    caller_proxy->issued = true;
    caller_proxy->requestor_id = http_requestor_id;
    caller_proxy->serial = serial;
    caller_proxy->proxy_port = PortPool::get_port();
    if (caller_proxy->proxy_port == 0) {
        RAISE_ERROR("No port is available to start HTTP caller proxy");
    }
    caller_proxy->setup_proxy_node();

    auto processor_proxy = dynamic_pointer_cast<BusCommandRouterProxy>(factory_empty_proxy());
    if (processor_proxy == nullptr) {
        RAISE_ERROR("Invalid proxy type for HTTP BUS_COMMAND_ROUTER dispatch");
    }
    processor_proxy->proxy_port = PortPool::get_port();
    if (processor_proxy->proxy_port == 0) {
        RAISE_ERROR("No port is available to start HTTP processor proxy");
    }
    processor_proxy->requestor_id = http_requestor_id;
    processor_proxy->serial = serial;
    const string requestor_host = http_requestor_id.substr(0, http_requestor_id.find(':'));
    const string processor_proxy_node_id = requestor_host + ":" + to_string(processor_proxy->proxy_port);
    processor_proxy->setup_proxy_node(processor_proxy_node_id, caller_proxy->my_id());
    processor_proxy->command = std::move(caller_proxy->command);
    processor_proxy->args = std::move(caller_proxy->args);

    this->run_command(processor_proxy);
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
    vector<string> lines;
    vector<string> tokens = proxy->parameters.tokenize();
    for (unsigned int i = 0; i + 2 < tokens.size(); i += 3) {
        lines.push_back(tokens[i] + ": " + tokens[i + 2]);
    }
    string response = Utils::join(lines, '\n');
    proxy->to_remote_peer(BusCommandRouterProxy::PARAMS_RESPONSE, {response});
    proxy->to_remote_peer(BaseProxy::FINISHED, {});
}

void BusCommandRouterProcessor::set_router_param(BusCommandRouterProxy* proxy,
                                                 const string& key,
                                                 const string& value) {
    auto iterator = proxy->parameters.find(key);
    if (iterator == proxy->parameters.end()) {
        RAISE_ERROR("Unknown parameter: '" + key + "'");
    }
    PropertyValue& current = iterator->second;

    auto type_mismatch = [&](const string& expected) {
        RAISE_ERROR("Parameter '" + key + "' is " + expected + "; cannot assign value: '" + value + "'");
    };

    if (std::holds_alternative<bool>(current)) {
        if (value == "true") {
            current = true;
        } else if (value == "false") {
            current = false;
        } else {
            type_mismatch("bool");
        }
    } else if (std::holds_alternative<unsigned int>(current)) {
        bool all_digits = !value.empty() && std::all_of(value.begin(), value.end(), [](char c) {
            return isdigit(static_cast<unsigned char>(c));
        });
        if (!all_digits) {
            type_mismatch("unsigned_int");
        }
        try {
            current = (unsigned int) std::stoul(value);
        } catch (const std::exception&) {
            type_mismatch("unsigned_int");
        }
    } else if (std::holds_alternative<long>(current)) {
        try {
            size_t consumed = 0;
            long parsed = std::stol(value, &consumed);
            if (consumed != value.size()) {
                type_mismatch("long");
            }
            current = parsed;
        } catch (const std::exception&) {
            type_mismatch("long");
        }
    } else if (std::holds_alternative<double>(current)) {
        try {
            size_t consumed = 0;
            double parsed = std::stod(value, &consumed);
            if (consumed != value.size()) {
                type_mismatch("double");
            }
            current = parsed;
        } catch (const std::exception&) {
            type_mismatch("double");
        }
    } else if (std::holds_alternative<string>(current)) {
        current = value;
    } else {
        RAISE_ERROR("Parameter '" + key + "' has an unsupported type for 'set'");
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
    key = Utils::trim(key);
    value = Utils::trim(value);
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

void BusCommandRouterProcessor::forward_to_service(shared_ptr<BusCommandRouterProxy> router_proxy,
                                                   shared_ptr<BaseQueryProxy> service_proxy) {
    shared_ptr<ServiceBus> bus =
        this->service_bus ? this->service_bus : ServiceBusSingleton::get_instance();
    bus->forward_bus_command(service_proxy, router_proxy->get_requestor_id(), router_proxy->peer_id());
    router_proxy->to_remote_peer(BusCommandRouterProxy::ROUTED, {});
}

void BusCommandRouterProcessor::handle_query(shared_ptr<BusCommandRouterProxy> proxy,
                                             const string& arg) {
    string context = proxy->parameters.get<string>(CONTEXT_KEY);
    string normalized_arg = normalize_metta_percent_variables(arg);
    vector<string> query_tokens;
    if (proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS)) {
        query_tokens = {normalized_arg};
    } else {
        query_tokens = Utils::split(normalized_arg, ' ');
    }

    auto pm_proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    pm_proxy->parameters = proxy->parameters;
    forward_to_service(proxy, pm_proxy);
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
    forward_to_service(proxy, evo_proxy);
}
