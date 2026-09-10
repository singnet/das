#pragma once

#include <nlohmann/json.hpp>
#include <string>

using namespace std;

namespace vault {

/**
 * Backend-agnostic KV client used to resolve vault:// config refs.
 */
class VaultClient {
   public:
    VaultClient(const VaultClient&) = delete;
    virtual ~VaultClient() = default;

    VaultClient& operator=(const VaultClient&) = delete;

    /** Secret payload. Raises if the secret cannot be read. */
    virtual nlohmann::json get_data(const string& mount, const string& path) = 0;

   protected:
    VaultClient() = default;
};

}  // namespace vault
