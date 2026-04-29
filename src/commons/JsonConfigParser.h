#pragma once

#include <string>

#include "JsonConfig.h"

using namespace std;

namespace commons {

/**
 * @brief Parses JSON config files with schema versioning: each version
 * declares required top-level fields; missing fields produce a clear error.
 */
class JsonConfigParser {
   public:
    /**
     * Load and validate config from a JSON file.
     * @param file_path Path to the JSON file (e.g. ~/.das/config.json).
     * @return JsonConfig instance with validated schema.
     * @param throw_flag Whether to throw an exception on file not found.
     */
    static JsonConfig load(const string& file_path, bool throw_flag = true);

    /**
     * Load and validate config from a JSON string (e.g. for tests).
     * @param json_content Raw JSON string.
     * @return JsonConfig instance with validated schema.
     * @throws std::runtime_error on invalid JSON or schema validation failure.
     */
    static JsonConfig load_from_string(const string& json_content);

    /**
     * Load and validate a bus_client config (schema 1.0): requires params and params.das_config_file
     * (path to `das.json`, same schema as bus `busnode` config). Atomdb may be supplied inline or merged
     * from that file in bus_client.
     */
    static JsonConfig load_client_config(const string& file_path, bool throw_flag = true);

    /** Same as load_client_config for inline JSON (tests). */
    static JsonConfig load_client_config_from_string(const string& json_content);
};

}  // namespace commons
