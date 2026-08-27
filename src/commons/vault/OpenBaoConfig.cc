#include "OpenBaoConfig.h"

#include <cstdlib>

#include "Utils.h"

using namespace commons;

namespace openbao {
namespace {

string require_string(const nlohmann::json& obj, const string& key, const string& context) {
    if (!obj.is_object() || !obj.contains(key) || !obj[key].is_string()) {
        RAISE_ERROR(context + ": missing or invalid '" + key + "'");
    }
    string value = obj[key].get<string>();
    if (value.empty()) {
        RAISE_ERROR(context + ": '" + key + "' must be non-empty");
    }
    return value;
}

}  // namespace

OpenBaoConfig OpenBaoConfig::from_json(const nlohmann::json& root) {
    if (!root.is_object() || !root.contains("vault")) {
        RAISE_ERROR("OpenBaoConfig: missing 'vault' section in config");
    }
    const nlohmann::json& vault = root["vault"];
    if (!vault.is_object()) {
        RAISE_ERROR("OpenBaoConfig: vault section must be an object");
    }

    string type = require_string(vault, "type", "OpenBaoConfig");
    if (type != "openbao") {
        RAISE_ERROR("OpenBaoConfig: vault.type must be \"openbao\" (got \"" + type + "\")");
    }

    string endpoint = require_string(vault, "endpoint", "OpenBaoConfig");

    OpenBaoConfig out;
    if (endpoint.find("://") == string::npos) {
        out.addr = "http://" + endpoint;
    } else {
        out.addr = endpoint;
    }
    return out;
}

void OpenBaoConfig::load_token_from_env() {
    const char* vault_token = getenv("VAULT_TOKEN");
    if (vault_token != nullptr && vault_token[0] != '\0') {
        token = vault_token;
        return;
    }
    const char* bao_token = getenv("BAO_TOKEN");
    if (bao_token != nullptr && bao_token[0] != '\0') {
        token = bao_token;
        return;
    }
    RAISE_ERROR(
        "OpenBaoConfig: vault:// refs require a token; set VAULT_TOKEN or BAO_TOKEN in the "
        "environment");
}

}  // namespace openbao
