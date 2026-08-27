#include "VaultJsonResolver.h"

#include <unordered_map>

#include "OpenBaoClient.h"
#include "OpenBaoConfig.h"
#include "Utils.h"

using namespace commons;

namespace openbao {
namespace {

bool is_vault_ref(const nlohmann::json& value) {
    return value.is_string() && value.get_ref<const string&>().rfind(VaultJsonResolver::kScheme, 0) == 0;
}

void walk_and_resolve(nlohmann::json& node,
                      const VaultJsonResolver::Fetcher& fetcher,
                      unordered_map<string, nlohmann::json>& cache) {
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            walk_and_resolve(it.value(), fetcher, cache);
        }
        return;
    }
    if (node.is_array()) {
        for (auto& element : node) {
            walk_and_resolve(element, fetcher, cache);
        }
        return;
    }
    if (!is_vault_ref(node)) {
        return;
    }

    string uri = node.get<string>();
    auto cached = cache.find(uri);
    if (cached != cache.end()) {
        node = cached->second;
        return;
    }

    auto mount_and_path = VaultJsonResolver::parse_uri(uri);
    nlohmann::json data = fetcher(mount_and_path.first, mount_and_path.second);
    nlohmann::json injected = VaultJsonResolver::inject_payload(data);
    cache.emplace(uri, injected);
    node = std::move(injected);
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

nlohmann::json VaultJsonResolver::inject_payload(const nlohmann::json& data) {
    if (!data.is_object()) {
        RAISE_ERROR("VaultJsonResolver: KV v2 data.data must be a JSON object");
    }
    if (data.empty()) {
        RAISE_ERROR("VaultJsonResolver: KV v2 data.data is empty");
    }
    if (data.size() == 1) {
        return data.begin().value();
    }
    return data;
}

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
        walk_and_resolve(it.value(), fetcher, cache);
    }
}

void VaultJsonResolver::resolve_with_openbao(nlohmann::json& root) {
    if (!has_vault_refs(root)) {
        return;
    }

    OpenBaoConfig config = OpenBaoConfig::from_json(root);
    config.load_token_from_env();
    OpenBaoClient client(config);

    resolve(root,
            [&client](const string& mount, const string& path) { return client.kv_data(mount, path); });
}

}  // namespace openbao
