#pragma once
#include <nlohmann/json.hpp>

#include "JsonConfig.h"

namespace atomdb {
/**
 * Minimal valid atomdb JsonConfig for local development / tests (all required fields present).
 * Production services should load values from the application config file instead.
 */
inline commons::JsonConfig test_atomdb_json_config() {
    return commons::JsonConfig(nlohmann::json::parse(R"({
        "redis": { "endpoint": "localhost:40020", "cluster": false },
        "mongodb": {
            "endpoint": "localhost:40021",
            "username": "admin",
            "password": "admin"
        },
        "morkdb": { "endpoint": "localhost:40022" }
    })"));
}
}  // namespace atomdb
