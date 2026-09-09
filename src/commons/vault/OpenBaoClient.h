#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "VaultClient.h"

using namespace std;

namespace httplib {
class Client;
}

namespace vault {

/**
 * OpenBao (Vault-compatible) KV v2 client.
 *
 *   mount "test", path "dbs/mongodb" -> GET /v1/test/data/dbs/mongodb
 */
class OpenBaoClient : public VaultClient {
   public:
    OpenBaoClient(string addr, string token);
    ~OpenBaoClient() override;

    OpenBaoClient(const OpenBaoClient&) = delete;
    OpenBaoClient& operator=(const OpenBaoClient&) = delete;

    nlohmann::json get_data(const string& mount, const string& path) override;

   private:
    string trim_path(const string& path) const;
    nlohmann::json get_json(const string& api_path);

    string addr_;
    string token_;
    unique_ptr<httplib::Client> cli_;
};

}  // namespace vault
