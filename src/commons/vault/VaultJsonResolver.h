#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

using namespace std;

namespace openbao {

/**
 * Resolves vault:// references inside a DAS JSON config tree.
 *
 * URI form:
 *   vault://<mount>/<secret-path>
 * Example:
 *   vault://test/dbs/mongodb_user  ->  GET /v1/test/data/dbs/mongodb_user
 *
 * KV v2 always stores an object in data.data. Injection rule:
 *   - Exactly one key  -> replace the config node with that key's value
 *     (so {"value":"admin"} or {"username":"admin"} becomes "admin")
 *   - Multiple keys    -> replace the config node with the whole object
 *
 * Strings inside the top-level "vault" block are never resolved.
 */
class VaultJsonResolver {
   public:
    /** (mount, secret_path) -> KV v2 data.data object */
    using Fetcher = function<nlohmann::json(const string& mount, const string& path)>;

    static constexpr const char* kScheme = "vault://";

    /**
     * Parse vault://<mount>/<path> into mount and path.
     * Raises on malformed URIs.
     */
    static pair<string, string> parse_uri(const string& uri);

    /**
     * Apply single-key unwrap vs whole-object inject rule to a KV data.data payload.
     */
    static nlohmann::json inject_payload(const nlohmann::json& data);

    /** True if any vault:// string exists outside the vault block. */
    static bool has_vault_refs(const nlohmann::json& root);

    /**
     * Walk root and replace every vault:// string with fetched/injected data.
     * Uses fetcher for KV reads; caches by full URI.
     * Does not modify root["vault"].
     */
    static void resolve(nlohmann::json& root, const Fetcher& fetcher);

    /**
     * Resolve using OpenBao at root["vault"].endpoint.
     * Prompts on the terminal for a token when vault:// refs exist.
     * No-op (no HTTP) when there are no vault:// refs.
     */
    static void resolve_with_openbao(nlohmann::json& root);
};

}  // namespace openbao
