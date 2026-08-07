#include "HttpCommandProxyFactory.h"

#include <vector>

#include "ProxyParametersFromJson.h"
#include "Utils.h"

using namespace command_router;
using namespace commons;

namespace {

bool parse_query_arg(const json& params, string& query_arg, string& error_message) {
    if (!params.contains("query") || !params["query"].is_object()) {
        error_message = "params.query must be an object";
        return false;
    }

    const json& query = params["query"];
    if (query.contains("syntax") &&
        (!query["syntax"].is_string() || query["syntax"].get_ref<const string&>() != "metta")) {
        error_message = "params.query.syntax must be \"metta\"";
        return false;
    }
    if (!query.contains("tokens") || !query["tokens"].is_array() || query["tokens"].empty()) {
        error_message = "params.query.tokens must be a non-empty array";
        return false;
    }

    vector<string> tokens;
    tokens.reserve(query["tokens"].size());
    for (const auto& token : query["tokens"]) {
        if (!token.is_string() || token.get_ref<const string&>().empty()) {
            error_message = "params.query.tokens entries must be non-empty strings";
            return false;
        }
        tokens.push_back(token.get<string>());
    }

    query_arg = Utils::join(tokens, ' ');
    Utils::replace_all(query_arg, "%", "$");
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

    string arg;

    if (command == QUERY) {
        if (!parse_query_arg(params, arg, error_message)) {
            return nullptr;
        }
    } else {
        error_message = "Unsupported command: " + command;
        return nullptr;
    }

    auto proxy = make_shared<BusCommandRouterProxy>(command, arg);

    if (!ProxyParametersFromJson::set(proxy->parameters, params, command, error_message)) {
        return nullptr;
    }

    return proxy;
}
