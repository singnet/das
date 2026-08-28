#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

using namespace std;

namespace vault {

/**
 * Resolves vault:// references inside a DAS JSON config tree.
 *
 * URI form:
 *   vault://<mount>/<secret-path>
 * Example:
 *   vault://test/db  ->  GET /v1/test/data/db
 *
 * KV v2 data.data must be an object whose keys match DAS config file keys.
 * A config node `"mongodb": "vault://test/db"` is replaced by data.data["mongodb"]
 * (string, object, or array). Missing keys are an error.
 * vault:// is not allowed as a JSON array element (no key).
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
     * Return data[key] from a KV v2 data.data object.
     * key is the das.json key being replaced. Raises if data is not an object
     * or does not contain key.
     */
    static nlohmann::json inject_payload(const nlohmann::json& data, const string& key);

    /** True if any vault:// string exists outside the vault block. */
    static bool has_vault_refs(const nlohmann::json& root);

    /**
     * Walk root and replace every vault:// string with data.data[<key>].
     * Uses fetcher for KV reads; caches raw payloads by URI.
     * Does not modify root["vault"].
     */
    static void resolve(nlohmann::json& root, const Fetcher& fetcher);

    /**
     * Resolve vault:// refs using root["vault"] (type + endpoint).
     * Prompts on the terminal for a token when refs exist.
     * No-op (no HTTP) when there are no vault:// refs.
     */
    static void resolve_from_config(nlohmann::json& root);
};

}  // namespace vault
