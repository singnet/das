#pragma once
#include <nlohmann/json.hpp>

#include "JsonConfig.h"

namespace atomdb {
/**
 * Minimal valid atomdb JsonConfig for local development / tests (all required fields present).
 * Production services should load values from the application config file instead.
 */
inline commons::JsonConfig test_atomdb_json_config(const string& atomdb_type = "redismongodb") {
    auto json = nlohmann::json();
    json["type"] = atomdb_type;
    json["redis"] = {{"endpoint", "localhost:40020"}, {"cluster", false}};
    json["mongodb"] = {{"endpoint", "localhost:40021"}, {"username", "admin"}, {"password", "admin"}};
    json["morkdb"] = {{"endpoint", "localhost:40022"}};
    return commons::JsonConfig(json);
}
}  // namespace atomdb
