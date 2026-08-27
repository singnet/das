#pragma once

#include <nlohmann/json.hpp>
#include <string>

using namespace std;

namespace openbao {

/**
 * Connection settings for an OpenBao HTTP client.
 * Loaded from the required "vault" section of a DAS JSON config:
 *
 *   vault.type      must be "openbao"
 *   vault.endpoint  host:port or full URL
 *
 * The client token is read from VAULT_TOKEN, then BAO_TOKEN — never from das.json.
 */
struct OpenBaoConfig {
    string addr;
    string token;

    /** Build from the root config JSON (reads the "vault" object). */
    static OpenBaoConfig from_json(const nlohmann::json& root);

    /** Resolve token from VAULT_TOKEN or BAO_TOKEN. Raises if neither is set. */
    void load_token_from_env();
};

}  // namespace openbao
