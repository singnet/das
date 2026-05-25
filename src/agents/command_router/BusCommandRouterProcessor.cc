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
const string CORRELATION_QUERIES_KEY = EVOLUTION_PARAM_CORRELATION_QUERIES;
const string CORRELATION_REPLACEMENTS_KEY = EVOLUTION_PARAM_CORRELATION_REPLACEMENTS;
const string CORRELATION_MAPPINGS_KEY = EVOLUTION_PARAM_CORRELATION_MAPPINGS;
const string FITNESS_FUNCTION_TAG_KEY = EVOLUTION_PARAM_FITNESS;

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

void BusCommandRouterProcessor::load_router_parameters(shared_ptr<BusCommandRouterProxy> proxy) {
    proxy->parameters = parameters_for_peer(proxy->get_requestor_id());
}

void BusCommandRouterProcessor::save_router_parameters(shared_ptr<BusCommandRouterProxy> proxy) {
    lock_guard<mutex> lock(this->router_parameters_mutex);
    this->router_parameters_by_peer[proxy->get_requestor_id()] = proxy->parameters;
}

void BusCommandRouterProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    auto router_proxy = dynamic_pointer_cast<BusCommandRouterProxy>(proxy);
    if (router_proxy == nullptr) {
        proxy->raise_error_on_peer("Invalid proxy type for BUS_COMMAND_ROUTER");
        return;
    }
    load_router_parameters(router_proxy);
    try {
        if (router_proxy->get_args().size() < 2) {
            RAISE_ERROR("Invalid bus_command_router args: expected {COMMAND, ARG}");
        }
        string command = router_proxy->get_args()[0];
        string arg = router_proxy->get_args()[1];
        dispatch(router_proxy, command, arg);
    } catch (const std::exception& exception) {
        router_proxy->raise_error_on_peer(exception.what());
    }
}

void BusCommandRouterProcessor::dispatch(shared_ptr<BusCommandRouterProxy> proxy,
                                         const string& command,
                                         const string& arg) {
    if (command == "get") {
        handle_get(proxy, arg);
    } else if (command == "set") {
        handle_set(proxy, arg);
    } else if (command == "query") {
        handle_query(proxy, arg);
    } else if (command == "evolution") {
        handle_evolution(proxy, arg);
    } else {
        RAISE_ERROR("Unknown router command: " + command);
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
        bool is_int = true;
        for (char c : value) {
            if (!isdigit(c)) {
                is_int = false;
                break;
            }
        }
        if (is_int && !value.empty()) {
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
    save_router_parameters(proxy);
    string ack = "ok " + key + " " + value;
    proxy->to_remote_peer(BusCommandRouterProxy::SET_PARAM_ACK, {ack});
    proxy->to_remote_peer(BaseProxy::FINISHED, {});
}

void BusCommandRouterProcessor::copy_query_parameters(shared_ptr<BaseProxy> target,
                                                      shared_ptr<BusCommandRouterProxy> router) {
    target->parameters = router->parameters;
}

string BusCommandRouterProcessor::parse_metta_expression(const string& metta_expression) {
    string parsed = metta_expression;
    Utils::replace_all(parsed, "%", "$");
    return parsed;
}

vector<string> BusCommandRouterProcessor::parse_request(const string& request_str, bool use_metta) {
    if (use_metta) {
        return {parse_metta_expression(request_str)};
    }
    return Utils::split(request_str, ' ');
}

vector<vector<string>> BusCommandRouterProcessor::parse_correlation_queries(const string& str,
                                                                            bool use_metta) {
    string parsed = use_metta ? parse_metta_expression(str) : str;
    vector<string> parts = Utils::split(parsed, ',');
    vector<vector<string>> queries;
    for (auto& q : parts) {
        Utils::trim(q);
        if (!q.empty()) {
            queries.push_back(parse_request(q, use_metta));
        }
    }
    return queries;
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
    bool use_metta = proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS);
    string context = proxy->parameters.get<string>(CONTEXT_KEY);
    vector<string> query_tokens = parse_request(arg, use_metta);

    auto pm_proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    copy_query_parameters(pm_proxy, proxy);
    pm_proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = use_metta;

    shared_ptr<ServiceBus> bus = this->service_bus;
    if (bus == nullptr) {
        bus = ServiceBusSingleton::get_instance();
    }
    bus->issue_bus_command(pm_proxy);

    proxy->to_remote_peer(BusCommandRouterProxy::ROUTED, {});

    thread relay_thread(
        [client_proxy = proxy, pm_proxy]() { relay_query_answers_to_client(client_proxy, pm_proxy); });
    relay_thread.detach();
}

void BusCommandRouterProcessor::handle_evolution(shared_ptr<BusCommandRouterProxy> proxy,
                                                 const string& arg) {
    bool use_metta = proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS);
    string context = proxy->parameters.get<string>(CONTEXT_KEY);

    string fitness_tag;
    string cq_str;
    string cr_str;
    string cm_str;
    vector<string> query;

    EvolutionMettaArgs metta_args;
    bool parsed_metta_arg = try_parse_evolution_metta_arg(arg, metta_args);
    if (parsed_metta_arg) {
        if (metta_args.query.empty()) {
            RAISE_ERROR("Evolution MeTTa ARG is missing (query ...)");
        }
        query = parse_request(metta_args.query, use_metta);
        fitness_tag = metta_args.fitness_function_tag;
    } else {
        query = parse_request(arg, use_metta);
        cq_str = proxy->parameters.get_or<string>(CORRELATION_QUERIES_KEY, "");
        cr_str = proxy->parameters.get_or<string>(CORRELATION_REPLACEMENTS_KEY, "");
        cm_str = proxy->parameters.get_or<string>(CORRELATION_MAPPINGS_KEY, "");
    }

    if (fitness_tag.empty()) {
        if (proxy->parameters.find(FITNESS_FUNCTION_TAG_KEY) == proxy->parameters.end()) {
            RAISE_ERROR("Missing router parameter: " + FITNESS_FUNCTION_TAG_KEY);
        }
        fitness_tag = proxy->parameters.get<string>(FITNESS_FUNCTION_TAG_KEY);
    }
    if (cq_str.empty()) {
        cq_str = proxy->parameters.get_or<string>(CORRELATION_QUERIES_KEY, "");
    }
    if (cr_str.empty()) {
        cr_str = proxy->parameters.get_or<string>(CORRELATION_REPLACEMENTS_KEY, "");
    }
    if (cm_str.empty()) {
        cm_str = proxy->parameters.get_or<string>(CORRELATION_MAPPINGS_KEY, "");
    }

    vector<vector<string>> correlation_queries;
    vector<map<string, QueryAnswerElement>> correlation_replacements;
    vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> correlation_mappings;

    if (parsed_metta_arg && !metta_args.correlation_query_expressions.empty()) {
        correlation_queries = metta_correlation_queries(metta_args.correlation_query_expressions);
    }
    if (parsed_metta_arg && !metta_args.correlation_replacement_groups.empty()) {
        correlation_replacements =
            metta_correlation_replacements(metta_args.correlation_replacement_groups);
    }
    if (parsed_metta_arg && !metta_args.correlation_mapping_groups.empty()) {
        correlation_mappings = metta_correlation_mappings(metta_args.correlation_mapping_groups);
    }
    if (correlation_queries.empty() && !cq_str.empty()) {
        if (use_metta) {
            correlation_queries = metta_correlation_queries(parse_correlation_query_list_body(cq_str));
        } else {
            correlation_queries = parse_correlation_queries(cq_str, false);
        }
    }
    if (correlation_replacements.empty() && !cr_str.empty()) {
        correlation_replacements =
            metta_correlation_replacements(parse_correlation_pair_groups_body(cr_str));
    }
    if (correlation_mappings.empty() && !cm_str.empty()) {
        correlation_mappings = metta_correlation_mappings(parse_correlation_pair_groups_body(cm_str));
    }

    if (!correlation_queries.empty() && !correlation_replacements.empty() &&
        correlation_queries.size() != correlation_replacements.size()) {
        RAISE_ERROR(
            "correlation-queries and correlation-replacements must have the same number of entries");
    }
    if (!correlation_mappings.empty() && !correlation_queries.empty() &&
        correlation_mappings.size() != correlation_queries.size()) {
        RAISE_ERROR("correlation-mappings must have one group per correlation-query");
    }

    auto evo_proxy = make_shared<QueryEvolutionProxy>(query,
                                                      correlation_queries,
                                                      correlation_replacements,
                                                      correlation_mappings,
                                                      context,
                                                      fitness_tag);
    copy_query_parameters(evo_proxy, proxy);

    shared_ptr<ServiceBus> bus = this->service_bus;
    if (bus == nullptr) {
        bus = ServiceBusSingleton::get_instance();
    }
    bus->issue_bus_command(evo_proxy);

    proxy->to_remote_peer(BusCommandRouterProxy::ROUTED, {});

    thread relay_thread(
        [client_proxy = proxy, evo_proxy]() { relay_query_answers_to_client(client_proxy, evo_proxy); });
    relay_thread.detach();
}
