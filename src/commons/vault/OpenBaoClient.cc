#define LOG_LEVEL INFO_LEVEL
#include "OpenBaoClient.h"

#include <httplib.h>

#include <memory>

#include "Logger.h"
#include "Utils.h"

using namespace commons;

namespace vault {

const int HTTP_TIMEOUT_SECONDS = 15;

OpenBaoClient::OpenBaoClient(string addr, string token)
    : addr_(std::move(addr)), token_(std::move(token)) {
    addr_ = Utils::trim(addr_);
    while (!addr_.empty() && addr_.back() == '/') {
        addr_.pop_back();
    }
    if (addr_.empty()) {
        RAISE_ERROR("OpenBaoClient: addr is empty");
    }
    if (token_.empty()) {
        RAISE_ERROR("OpenBaoClient: no token");
    }

    cli_ = make_unique<httplib::Client>(addr_);
    cli_->set_connection_timeout(HTTP_TIMEOUT_SECONDS);
    cli_->set_read_timeout(HTTP_TIMEOUT_SECONDS);
    cli_->set_write_timeout(HTTP_TIMEOUT_SECONDS);
}

OpenBaoClient::~OpenBaoClient() = default;

string OpenBaoClient::trim_path(const string& path) const {
    string trimmed = Utils::trim(path);
    while (!trimmed.empty() && trimmed.front() == '/') {
        trimmed.erase(trimmed.begin());
    }
    while (!trimmed.empty() && trimmed.back() == '/') {
        trimmed.pop_back();
    }
    return trimmed;
}

nlohmann::json OpenBaoClient::get_json(const string& api_path) {
    httplib::Headers headers;
    headers.emplace("Accept", "application/json");
    headers.emplace("X-Vault-Request", "true");
    headers.emplace("X-Vault-Token", token_);

    auto result = cli_->Get("/v1/" + api_path, headers);
    if (!result) {
        RAISE_ERROR("OpenBaoClient: failed to reach " + addr_ + "/v1/" + api_path);
    }

    nlohmann::json body = nullptr;
    if (!result->body.empty()) {
        try {
            body = nlohmann::json::parse(result->body);
        } catch (const nlohmann::json::parse_error& e) {
            RAISE_ERROR("OpenBaoClient: invalid JSON from /v1/" + api_path + ": " + e.what());
        }
    }

    LOG_DEBUG("OpenBao GET /v1/" << api_path << " -> " << result->status);
    if (result->status != 200) {
        string message =
            "OpenBao GET /v1/" + api_path + " failed with HTTP " + std::to_string(result->status);
        if (body.is_object() && body.contains("errors") && body["errors"].is_array()) {
            message += ": " + body["errors"].dump();
        } else if (!result->body.empty()) {
            message += ": " + result->body;
        }
        RAISE_ERROR(message);
    }
    return body;
}

nlohmann::json OpenBaoClient::get_data(const string& mount, const string& path) {
    string api_path = trim_path(mount) + "/data/" + trim_path(path);
    nlohmann::json body = get_json(api_path);
    if (body.is_object() && body.contains("data") && body["data"].is_object() &&
        body["data"].contains("data")) {
        return body["data"]["data"];
    }
    RAISE_ERROR("OpenBaoClient: KV v2 payload missing data.data for path " + mount + "/" + path);
}

}  // namespace vault
