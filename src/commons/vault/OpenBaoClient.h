#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using namespace std;

namespace httplib {
class Client;
}

namespace openbao {

/**
 * HTTP client for OpenBao KV v2 reads used to resolve vault:// config refs.
 *
 *   mount "test", path "dbs/mongodb" -> GET /v1/test/data/dbs/mongodb
 */
class OpenBaoClient {
   public:
    OpenBaoClient(string addr, string token);
    ~OpenBaoClient();

    OpenBaoClient(const OpenBaoClient&) = delete;
    OpenBaoClient& operator=(const OpenBaoClient&) = delete;

    /** KV v2 secret payload only (data.data). Raises on non-200. */
    nlohmann::json kv_data(const string& mount, const string& path);

   private:
    string trim_path(const string& path) const;
    nlohmann::json get_json(const string& api_path);

    string addr_;
    string token_;
    unique_ptr<httplib::Client> cli_;
};

}  // namespace openbao
