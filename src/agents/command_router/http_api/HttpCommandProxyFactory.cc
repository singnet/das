#include "HttpCommandProxyFactory.h"

#include <vector>

#include "BaseQueryProxy.h"
#include "ProxyParametersFromJson.h"
#include "Utils.h"

using namespace command_router;
using namespace commons;

namespace {

string quote_metta_token(const string& value) {
    string escaped;
    escaped.reserve(value.size() + 2);
    for (char c : value) {
        if (c == '\\' || c == '"') {
            escaped.push_back('\\');
        }
        escaped.push_back(c);
    }
    return "\"" + escaped + "\"";
}

bool parse_query_tokens_object(
    const json& object, const string& path, bool use_metta, string& expression, string& error_message) {
    if (!object.is_object()) {
        error_message = path + " must be an object";
        return false;
    }
    if (!object.contains("tokens") || !object["tokens"].is_array() || object["tokens"].empty()) {
        error_message = path + ".tokens must be a non-empty array";
        return false;
    }

    vector<string> tokens;
    tokens.reserve(object["tokens"].size());
    for (const auto& token : object["tokens"]) {
        if (!token.is_string() || token.get_ref<const string&>().empty()) {
            error_message = path + ".tokens entries must be non-empty strings";
            return false;
        }
        tokens.push_back(token.get<string>());
    }

    expression = Utils::join(tokens, ' ');
    if (use_metta) {
        Utils::replace_all(expression, "%", "$");
    }
    return true;
}

bool parse_query_arg(const json& params, bool use_metta, string& query_arg, string& error_message) {
    if (!params.contains("query")) {
        error_message = "params.query must be an object";
        return false;
    }
    return parse_query_tokens_object(
        params["query"], "params.query", use_metta, query_arg, error_message);
}

bool parse_pair_groups(const json& groups_json, const string& path, string& out, string& error_message) {
    if (!groups_json.is_array()) {
        error_message = path + " must be an array of pair-groups";
        return false;
    }

    out = "(";
    for (size_t g = 0; g < groups_json.size(); ++g) {
        const json& group = groups_json[g];
        if (!group.is_array()) {
            error_message = path + "[" + std::to_string(g) + "] must be an array of pairs";
            return false;
        }
        out += "(";
        for (size_t p = 0; p < group.size(); ++p) {
            const json& pair = group[p];
            if (!pair.is_array() || pair.size() != 2 || !pair[0].is_string() || !pair[1].is_string()) {
                error_message = path + "[" + std::to_string(g) + "][" + std::to_string(p) +
                                "] must be a [string, string] pair";
                return false;
            }
            if (pair[0].get_ref<const string&>().empty() || pair[1].get_ref<const string&>().empty()) {
                error_message = path + " pair entries must be non-empty strings";
                return false;
            }
            out += "(" + quote_metta_token(pair[0].get<string>()) + " " +
                   quote_metta_token(pair[1].get<string>()) + ")";
            if (p + 1 < group.size()) {
                out += " ";
            }
        }
        out += ")";
        if (g + 1 < groups_json.size()) {
            out += " ";
        }
    }
    out += ")";
    return true;
}

bool parse_evolution_arg(const json& params,
                         bool use_metta,
                         string& evolution_arg,
                         string& error_message) {
    if (!params.contains("evolution") || !params["evolution"].is_object()) {
        error_message = "params.evolution must be an object";
        return false;
    }

    const json& evolution = params["evolution"];

    string query_expr;
    if (!evolution.contains("query")) {
        error_message = "params.evolution.query must be an object";
        return false;
    }
    if (!parse_query_tokens_object(
            evolution["query"], "params.evolution.query", use_metta, query_expr, error_message)) {
        return false;
    }

    if (!evolution.contains("fitness_function_tag") || !evolution["fitness_function_tag"].is_string() ||
        evolution["fitness_function_tag"].get_ref<const string&>().empty()) {
        error_message = "params.evolution.fitness_function_tag must be a non-empty string";
        return false;
    }
    const string fitness_tag = evolution["fitness_function_tag"].get<string>();
    // The tag is concatenated raw into the MeTTa ARG as (ff <tag>); delimiters would
    // change the parsed structure or break registry lookup.
    if (fitness_tag.find_first_of(" \t\r\n()\"") != string::npos) {
        error_message =
            "params.evolution.fitness_function_tag must not contain whitespace, parentheses,"
            " or quotes";
        return false;
    }

    const string query_body = use_metta ? query_expr : quote_metta_token(query_expr);
    string arg = "((query " + query_body + ") (ff " + fitness_tag + ")";

    if (evolution.contains("correlation_queries")) {
        if (!evolution["correlation_queries"].is_array()) {
            error_message = "params.evolution.correlation_queries must be an array";
            return false;
        }
        arg += " (cq (";
        const auto& cq = evolution["correlation_queries"];
        for (size_t i = 0; i < cq.size(); ++i) {
            string cq_expr;
            if (!parse_query_tokens_object(
                    cq[i],
                    "params.evolution.correlation_queries[" + std::to_string(i) + "]",
                    use_metta,
                    cq_expr,
                    error_message)) {
                return false;
            }
            arg += use_metta ? cq_expr : quote_metta_token(cq_expr);
            if (i + 1 < cq.size()) {
                arg += " ";
            }
        }
        arg += "))";
    }

    if (evolution.contains("correlation_replacements")) {
        string cr_body;
        if (!parse_pair_groups(evolution["correlation_replacements"],
                               "params.evolution.correlation_replacements",
                               cr_body,
                               error_message)) {
            return false;
        }
        arg += " (cr " + cr_body + ")";
    }

    if (evolution.contains("correlation_mappings")) {
        string cm_body;
        if (!parse_pair_groups(evolution["correlation_mappings"],
                               "params.evolution.correlation_mappings",
                               cm_body,
                               error_message)) {
            return false;
        }
        arg += " (cm " + cm_body + ")";
    }

    arg += ")";
    evolution_arg = arg;
    return true;
}

}  // namespace

shared_ptr<BusCommandRouterProxy> HttpCommandProxyFactory::create(const string& command,
                                                                  const json& params,
                                                                  string& error_message) {
    if (!params.is_object()) {
        error_message = "params must be an object";
        return nullptr;
    }

    auto proxy = make_shared<BusCommandRouterProxy>(command, "");
    if (!ProxyParametersFromJson::set(proxy->parameters, params, command, error_message)) {
        return nullptr;
    }
    const bool use_metta = proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS);

    string arg;
    if (command == QUERY) {
        if (!parse_query_arg(params, use_metta, arg, error_message)) {
            return nullptr;
        }
    } else if (command == EVOLUTION) {
        if (!parse_evolution_arg(params, use_metta, arg, error_message)) {
            return nullptr;
        }
    } else {
        error_message = "Unsupported command: " + command;
        return nullptr;
    }

    proxy->args = {command, arg};
    return proxy;
}
