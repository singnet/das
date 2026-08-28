#include "VaultJsonResolver.h"

#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <unordered_map>

#include "OpenBaoClient.h"
#include "Utils.h"

using namespace commons;

namespace vault {
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

struct HideStdinEcho {
    termios previous{};
    bool active = false;

    HideStdinEcho() {
        if (tcgetattr(STDIN_FILENO, &previous) != 0) return;
        termios hidden = previous;
        hidden.c_lflag &= static_cast<tcflag_t>(~ECHO);
        if (tcsetattr(STDIN_FILENO, TCSANOW, &hidden) == 0) {
            active = true;
        }
    }

    ~HideStdinEcho() {
        if (active) {
            tcsetattr(STDIN_FILENO, TCSANOW, &previous);
        }
    }
};

string prompt_for_token() {
    if (!isatty(STDIN_FILENO)) {
        RAISE_ERROR(
            "VaultJsonResolver: vault:// refs require a token entered in the terminal "
            "(stdin is not a TTY)");
    }

    cerr << "Vault token: " << flush;
    string token;
    {
        HideStdinEcho hide_echo;
        if (!getline(cin, token)) {
            cerr << endl;
            RAISE_ERROR("VaultJsonResolver: failed to read vault token from the terminal");
        }
    }
    cerr << endl;

    token = Utils::trim(token);
    if (token.empty()) {
        RAISE_ERROR("VaultJsonResolver: vault token must be non-empty");
    }
    return token;
}

string vault_endpoint_from_config(const nlohmann::json& vault) {
    string endpoint = require_string(vault, "endpoint", "VaultJsonResolver");
    if (endpoint.find("://") == string::npos) {
        RAISE_ERROR("VaultJsonResolver: vault.endpoint must include a scheme, e.g. \"http://" +
                    endpoint + "\" or \"https://" + endpoint + "\"");
    }
    return endpoint;
}

unique_ptr<VaultClient> make_vault_client(const nlohmann::json& root) {
    if (!root.is_object() || !root.contains("vault") || !root["vault"].is_object()) {
        RAISE_ERROR("VaultJsonResolver: missing 'vault' section in config");
    }
    const nlohmann::json& vault_section = root["vault"];
    string type = require_string(vault_section, "type", "VaultJsonResolver");
    if (type == "openbao") {
        string addr = vault_endpoint_from_config(vault_section);
        return make_unique<OpenBaoClient>(std::move(addr), prompt_for_token());
    }
    RAISE_ERROR("VaultJsonResolver: unsupported vault.type \"" + type +
                "\" (only openbao is supported)");
}

bool is_vault_ref(const nlohmann::json& value) {
    return value.is_string() && value.get_ref<const string&>().rfind(VaultJsonResolver::kScheme, 0) == 0;
}

void walk_and_resolve(nlohmann::json& node,
                      const VaultJsonResolver::Fetcher& fetcher,
                      unordered_map<string, nlohmann::json>& cache,
                      const string& key) {
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            walk_and_resolve(it.value(), fetcher, cache, it.key());
        }
        return;
    }
    if (node.is_array()) {
        for (auto& element : node) {
            if (is_vault_ref(element)) {
                RAISE_ERROR(
                    "VaultJsonResolver: vault:// is not allowed as an array element "
                    "(KV key is taken from the DAS config file key)");
            }
            walk_and_resolve(element, fetcher, cache, key);
        }
        return;
    }
    if (!is_vault_ref(node)) {
        return;
    }
    if (key.empty()) {
        RAISE_ERROR("VaultJsonResolver: vault:// ref is missing a DAS config file key");
    }

    string uri = node.get<string>();
    auto cached = cache.find(uri);
    if (cached == cache.end()) {
        auto mount_and_path = VaultJsonResolver::parse_uri(uri);
        cached = cache.emplace(uri, fetcher(mount_and_path.first, mount_and_path.second)).first;
    }
    node = VaultJsonResolver::inject_payload(cached->second, key);
}

bool scan_for_refs(const nlohmann::json& node) {
    if (is_vault_ref(node)) {
        return true;
    }
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            if (scan_for_refs(it.value())) return true;
        }
    } else if (node.is_array()) {
        for (const auto& element : node) {
            if (scan_for_refs(element)) return true;
        }
    }
    return false;
}

}  // namespace

// -------------------------------------------------------------------------------------------------
// URI and payload handling
// -------------------------------------------------------------------------------------------------
pair<string, string> VaultJsonResolver::parse_uri(const string& uri) {
    if (uri.rfind(kScheme, 0) != 0) {
        RAISE_ERROR("VaultJsonResolver: not a vault:// URI: " + uri);
    }
    string rest = uri.substr(string(kScheme).size());
    while (!rest.empty() && rest.front() == '/') {
        rest.erase(rest.begin());
    }
    if (rest.empty()) {
        RAISE_ERROR("VaultJsonResolver: empty path in URI: " + uri);
    }

    size_t slash = rest.find('/');
    if (slash == string::npos || slash == 0 || slash + 1 >= rest.size()) {
        RAISE_ERROR("VaultJsonResolver: URI must be vault://<mount>/<secret-path> (got \"" + uri +
                    "\")");
    }

    string mount = rest.substr(0, slash);
    string path = rest.substr(slash + 1);
    while (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    if (mount.empty() || path.empty()) {
        RAISE_ERROR("VaultJsonResolver: URI must be vault://<mount>/<secret-path> (got \"" + uri +
                    "\")");
    }
    return {mount, path};
}

nlohmann::json VaultJsonResolver::inject_payload(const nlohmann::json& data, const string& key) {
    if (!data.is_object()) {
        RAISE_ERROR("VaultJsonResolver: KV data must be a JSON object");
    }
    if (key.empty()) {
        RAISE_ERROR("VaultJsonResolver: missing DAS config file key for vault payload");
    }
    auto it = data.find(key);
    if (it == data.end()) {
        RAISE_ERROR("VaultJsonResolver: KV payload missing key \"" + key +
                    "\" (keys in the secret must match DAS config file)");
    }
    return *it;
}

// -------------------------------------------------------------------------------------------------
// Resolution API
// -------------------------------------------------------------------------------------------------
bool VaultJsonResolver::has_vault_refs(const nlohmann::json& root) {
    if (!root.is_object()) {
        return scan_for_refs(root);
    }
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (it.key() == "vault") continue;
        if (scan_for_refs(it.value())) return true;
    }
    return false;
}

void VaultJsonResolver::resolve(nlohmann::json& root, const Fetcher& fetcher) {
    if (!root.is_object()) {
        RAISE_ERROR("VaultJsonResolver: config root must be a JSON object");
    }
    unordered_map<string, nlohmann::json> cache;
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (it.key() == "vault") continue;
        walk_and_resolve(it.value(), fetcher, cache, it.key());
    }
}

void VaultJsonResolver::resolve_from_config(nlohmann::json& root) {
    if (!has_vault_refs(root)) {
        return;
    }

    unique_ptr<VaultClient> client = make_vault_client(root);

    resolve(root, [&client](const string& mount, const string& path) {
        return client->get_data(mount, path);
    });
}

}  // namespace vault
