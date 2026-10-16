#include "BusCommandRouterProcessor.h"

#include <algorithm>
#include <atomic>
#include <variant>

#include "AtomDBSingleton.h"
#include "AtomExpression.h"
#include "BaseQueryProxy.h"
#include "EvolutionMettaParser.h"
#include "Link.h"
#include "LinkCreationProxy.h"
#include "Node.h"
#include "PatternMatchingQueryProxy.h"
#include "PortPool.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBusSingleton.h"
#include "SystemParametersSingleton.h"
#include "Utils.h"
#include "nlohmann/json.hpp"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace command_router;
using namespace query_engine;
using namespace evolution;
using namespace link_creation_agent;
using namespace service_bus;
using namespace commons;
using namespace atoms;

using json = nlohmann::json;

namespace {

const string CONTEXT_KEY = "context";
atomic<unsigned int> HTTP_REQUEST_SERIAL{1};

// Copy HTTP/router overrides onto a QueryEvolutionProxy the same way a bus
// client does: keep evolution-agent defaults, apply query+evolution keys, and
// drop context-agent keys (use_cache, initial_rent_rate, ...). Replacing the
// whole map leaked those keys into sampling queries and changed attention.
void apply_direct_evolution_parameters(Properties& evo_parameters, const Properties& router_parameters) {
    Properties allowed = SystemParametersSingleton::get_instance()->get_evolution_agent_params() +
                         SystemParametersSingleton::get_instance()->get_query_agent_params();
    for (const auto& entry : router_parameters) {
        if (allowed.find(entry.first) != allowed.end()) {
            evo_parameters[entry.first] = entry.second;
        }
    }
}

// Keep link-creation defaults, then overlay query + link-creation keys. Context-agent
// keys (use_cache, initial_rent_rate, ...) stay off the downstream proxy.
void apply_direct_link_creation_parameters(Properties& lc_parameters,
                                           const Properties& router_parameters) {
    Properties allowed = SystemParametersSingleton::get_instance()->get_link_creation_agent_params() +
                         SystemParametersSingleton::get_instance()->get_query_agent_params();
    allowed[LinkCreationProxy::LOG_NEW_LINKS] = true;
    allowed[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] = string("");
    allowed[LinkCreationProxy::LINK_CREATOR_EXTRA_PARAMETERS] = string("");
    for (const auto& entry : router_parameters) {
        if (allowed.find(entry.first) != allowed.end()) {
            lc_parameters[entry.first] = entry.second;
        }
    }
}

}  // namespace

BusCommandRouterProcessor::BusCommandRouterProcessor(shared_ptr<ServiceBus> service_bus)
    : BusCommandProcessor({ServiceBus::BUS_COMMAND_ROUTER}), service_bus(service_bus) {
    this->atomdb = AtomDBSingleton::get_instance();
}

BusCommandRouterProcessor::~BusCommandRouterProcessor() {
    this->atomdb = AtomDBSingleton::get_instance();
}

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

void BusCommandRouterProcessor::prepare_http_caller_proxy(
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

    caller_proxy->issued = true;
    caller_proxy->requestor_id = http_requestor_id;
    caller_proxy->serial = HTTP_REQUEST_SERIAL.fetch_add(1);
    caller_proxy->proxy_port = PortPool::get_port();
    if (caller_proxy->proxy_port == 0) {
        RAISE_ERROR("No port is available to start HTTP caller proxy");
    }
    caller_proxy->setup_proxy_node();
}

void BusCommandRouterProcessor::dispatch_http_query(
    const shared_ptr<BusCommandRouterProxy>& caller_proxy, const string& http_requestor_id) {
    this->prepare_http_caller_proxy(caller_proxy, http_requestor_id);
    try {
        if (caller_proxy->get_args().size() < 2 || caller_proxy->get_args()[0] != "query") {
            RAISE_ERROR("HTTP query requires args {query, ARG}");
        }
        this->handle_query(caller_proxy, caller_proxy->get_args()[1], caller_proxy->my_id(), false);
        caller_proxy->routed_flag = true;
    } catch (const std::exception& exception) {
        caller_proxy->raise_error(exception.what());
    }
}

void BusCommandRouterProcessor::dispatch_http_command(
    const shared_ptr<BusCommandRouterProxy>& caller_proxy, const string& http_requestor_id) {
    this->prepare_http_caller_proxy(caller_proxy, http_requestor_id);

    auto processor_proxy = dynamic_pointer_cast<BusCommandRouterProxy>(factory_empty_proxy());
    if (processor_proxy == nullptr) {
        RAISE_ERROR("Invalid proxy type for HTTP BUS_COMMAND_ROUTER dispatch");
    }
    processor_proxy->proxy_port = PortPool::get_port();
    if (processor_proxy->proxy_port == 0) {
        RAISE_ERROR("No port is available to start HTTP processor proxy");
    }
    processor_proxy->requestor_id = http_requestor_id;
    processor_proxy->serial = caller_proxy->serial;
    const string requestor_host = http_requestor_id.substr(0, http_requestor_id.find(':'));
    const string processor_proxy_node_id = requestor_host + ":" + to_string(processor_proxy->proxy_port);
    processor_proxy->setup_proxy_node(processor_proxy_node_id, caller_proxy->my_id());
    processor_proxy->command = std::move(caller_proxy->command);
    processor_proxy->args = std::move(caller_proxy->args);
    processor_proxy->parameters = caller_proxy->parameters;

    this->run_command_internal(processor_proxy, false);
}

void BusCommandRouterProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    auto router_proxy = dynamic_pointer_cast<BusCommandRouterProxy>(proxy);
    if (router_proxy == nullptr) {
        proxy->raise_error_on_peer("Invalid proxy type for BUS_COMMAND_ROUTER");
        return;
    }
    this->run_command_internal(router_proxy, true);
}

void BusCommandRouterProcessor::run_command_internal(shared_ptr<BusCommandRouterProxy> router_proxy,
                                                     bool load_peer_parameters) {
    if (load_peer_parameters) {
        router_proxy->parameters = parameters_for_peer(router_proxy->get_requestor_id());
    }
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
            handle_query(router_proxy, arg, router_proxy->peer_id(), true);
        } else if (command == "evolution") {
            handle_evolution(router_proxy, arg);
        } else if (command == "link_creation") {
            handle_link_creation(router_proxy, arg);
        } else if (command == "add_atom") {
            handle_add_atom(router_proxy, arg);
        } else if (command == "delete_atom") {
            handle_delete_atom(router_proxy, arg);
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
                                                   shared_ptr<BaseQueryProxy> service_proxy,
                                                   const string& answer_node_id,
                                                   bool notify_routed) {
    shared_ptr<ServiceBus> bus =
        this->service_bus ? this->service_bus : ServiceBusSingleton::get_instance();
    bus->forward_bus_command(service_proxy, router_proxy->get_requestor_id(), answer_node_id);
    if (notify_routed) {
        router_proxy->to_remote_peer(BusCommandRouterProxy::ROUTED, {});
    }
}

void BusCommandRouterProcessor::handle_query(shared_ptr<BusCommandRouterProxy> proxy,
                                             const string& arg,
                                             const string& answer_node_id,
                                             bool notify_routed) {
    string context = proxy->parameters.get<string>(CONTEXT_KEY);
    vector<string> query_tokens;
    if (proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS)) {
        query_tokens = {normalize_metta_percent_variables(arg)};
    } else {
        query_tokens = Utils::split(arg, ' ');
    }

    auto pm_proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    pm_proxy->parameters = proxy->parameters;
    this->forward_to_service(proxy, pm_proxy, answer_node_id, notify_routed);
}

void BusCommandRouterProcessor::handle_evolution(shared_ptr<BusCommandRouterProxy> proxy,
                                                 const string& arg) {
    string context = proxy->parameters.get<string>(CONTEXT_KEY);

    EvolutionMettaArgs metta_args;
    if (!try_parse_evolution_metta_arg(arg, metta_args) || metta_args.query.empty()) {
        RAISE_ERROR("Evolution ARG must be a labeled MeTTa form starting with (query ...)");
    }

    string fitness_tag = metta_args.fitness_function_tag;
    if (fitness_tag.empty()) {
        RAISE_ERROR("Missing fitness function tag in Evolution ARG: " + arg);
    }

    const bool use_metta = proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS);

    vector<string> query;
    vector<vector<string>> correlation_queries;
    if (use_metta) {
        query = {normalize_metta_percent_variables(metta_args.query)};
        correlation_queries = metta_correlation_queries(metta_args.correlation_query_expressions);
    } else {
        query = Utils::split(metta_args.query, ' ');
        correlation_queries.reserve(metta_args.correlation_query_expressions.size());
        for (const auto& expression : metta_args.correlation_query_expressions) {
            correlation_queries.push_back(Utils::split(expression, ' '));
        }
    }
    auto correlation_replacements =
        metta_correlation_replacements(metta_args.correlation_replacement_groups);
    auto correlation_mappings = metta_correlation_mappings(metta_args.correlation_mapping_groups);

    auto evo_proxy = make_shared<QueryEvolutionProxy>(query,
                                                      correlation_queries,
                                                      correlation_replacements,
                                                      correlation_mappings,
                                                      context,
                                                      fitness_tag);
    apply_direct_evolution_parameters(evo_proxy->parameters, proxy->parameters);
    this->forward_to_service(proxy, evo_proxy, proxy->peer_id(), true);
}

void BusCommandRouterProcessor::handle_link_creation(shared_ptr<BusCommandRouterProxy> proxy,
                                                     const string& arg) {
    string context = proxy->parameters.get<string>(CONTEXT_KEY);

    LinkCreationMettaArgs metta_args;
    if (!try_parse_link_creation_metta_arg(arg, metta_args)) {
        RAISE_ERROR(
            "Link creation ARG must be a labeled MeTTa form ((query ...) (lc <link-creator-tag>))");
    }

    const bool use_metta = proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS);
    vector<string> query_tokens;
    if (use_metta) {
        query_tokens = {normalize_metta_percent_variables(metta_args.query)};
    } else {
        query_tokens = Utils::split(metta_args.query, ' ');
    }

    auto lc_proxy = make_shared<LinkCreationProxy>(
        query_tokens, context, metta_args.link_creator_tag, BaseProxy::NONE);
    apply_direct_link_creation_parameters(lc_proxy->parameters, proxy->parameters);
    this->forward_to_service(proxy, lc_proxy, proxy->peer_id(), true);
}

namespace {

json parse_atom_arg(const string& arg) {
    json spec = json::parse(arg);
    if (!spec.is_object()) {
        RAISE_ERROR("Atom ARG must be a JSON object");
    }
    return spec;
}

bool optional_bool(const json& spec, const string& key, bool default_value) {
    if (!spec.contains(key)) {
        return default_value;
    }
    if (!spec[key].is_boolean()) {
        RAISE_ERROR(key + " must be a bool");
    }
    return spec[key].get<bool>();
}

}  // namespace

string BusCommandRouterProcessor::edit_atom(const string& command, const string& arg) {
    if (command == "add_atom") {
        return this->add_atom(arg);
    }
    if (command == "delete_atom") {
        return this->delete_atom(arg);
    }
    RAISE_ERROR("Not an atom edit command: " + command);
    return "";
}

void BusCommandRouterProcessor::handle_add_atom(shared_ptr<BusCommandRouterProxy> proxy,
                                                const string& arg) {
    const string handle = this->add_atom(arg);
    proxy->to_remote_peer(BusCommandRouterProxy::ATOM_RESULT, {handle});
    proxy->to_remote_peer(BaseProxy::FINISHED, {});
}

string BusCommandRouterProcessor::add_atom(const string& arg) {
    json spec = parse_atom_arg(arg);
    if (spec.contains("metta")) {
        if (!spec["metta"].is_string() || spec["metta"].get_ref<const string&>().empty()) {
            RAISE_ERROR("add_atom.metta must be a non-empty string");
        }
        if (spec.contains("kind")) {
            RAISE_ERROR("add_atom.metta cannot be combined with kind");
        }
        return add_metta_expression(*this->atomdb, spec["metta"].get<string>());
    }
    if (!spec.contains("kind") || !spec["kind"].is_string()) {
        RAISE_ERROR("add_atom.kind must be \"node\" or \"link\"");
    }
    const string kind = spec["kind"].get<string>();
    if (!spec.contains("atom_type") || !spec["atom_type"].is_string() ||
        spec["atom_type"].get_ref<const string&>().empty()) {
        RAISE_ERROR("add_atom.atom_type must be a non-empty string");
    }
    const string atom_type = spec["atom_type"].get<string>();

    string handle;
    if (kind == "node") {
        if (!spec.contains("name") || !spec["name"].is_string() ||
            spec["name"].get_ref<const string&>().empty()) {
            RAISE_ERROR("add_atom.name must be a non-empty string");
        }
        Node node(atom_type, spec["name"].get<string>(), optional_bool(spec, "is_toplevel", false));
        handle = this->atomdb->add_node(&node);
    } else if (kind == "link") {
        if (!spec.contains("targets") || !spec["targets"].is_array() || spec["targets"].empty()) {
            RAISE_ERROR("add_atom.targets must be a non-empty array of strings");
        }
        vector<string> targets;
        targets.reserve(spec["targets"].size());
        for (const auto& target : spec["targets"]) {
            if (!target.is_string() || target.get_ref<const string&>().empty()) {
                RAISE_ERROR("add_atom.targets entries must be non-empty strings");
            }
            targets.push_back(target.get<string>());
        }
        Link link(atom_type, targets, optional_bool(spec, "is_toplevel", true));
        handle = this->atomdb->add_link(&link);
    } else {
        RAISE_ERROR("add_atom.kind must be \"node\" or \"link\"");
    }
    if (handle.empty()) {
        RAISE_ERROR("AtomDB add returned an empty handle");
    }
    return handle;
}

void BusCommandRouterProcessor::handle_delete_atom(shared_ptr<BusCommandRouterProxy> proxy,
                                                   const string& arg) {
    const string result = this->delete_atom(arg);
    proxy->to_remote_peer(BusCommandRouterProxy::ATOM_RESULT, {result});
    proxy->to_remote_peer(BaseProxy::FINISHED, {});
}

string BusCommandRouterProcessor::delete_atom(const string& arg) {
    json spec = parse_atom_arg(arg);
    const bool delete_link_targets = optional_bool(spec, "delete_link_targets", false);
    string handle;
    if (spec.contains("metta")) {
        if (!spec["metta"].is_string() || spec["metta"].get_ref<const string&>().empty()) {
            RAISE_ERROR("delete_atom.metta must be a non-empty string");
        }
        if (spec.contains("handle")) {
            RAISE_ERROR("delete_atom.metta cannot be combined with handle");
        }
        handle = metta_expression_root_handle(spec["metta"].get<string>());
    } else if (!spec.contains("handle") || !spec["handle"].is_string() ||
               spec["handle"].get_ref<const string&>().empty()) {
        RAISE_ERROR("delete_atom.handle must be a non-empty string");
    } else {
        handle = spec["handle"].get<string>();
    }
    const bool deleted = this->atomdb->delete_atom(handle, delete_link_targets);
    return deleted ? "deleted" : "not_found";
}
