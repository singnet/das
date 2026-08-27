#pragma once

#include <string>

#include "JsonConfig.h"

using namespace std;

namespace commons {

/**
 * @brief Parses JSON config files with schema versioning: each version
 * declares required top-level fields; missing fields produce a clear error.
 *
 * Required fields include vault, vault.type, and vault.endpoint.
 * Any string value starting with vault:// is replaced by the KV payload from
 * OpenBao. The token is entered on the terminal (not stored in das.json).
 * vault.type selects the backend (currently "openbao").
 * If there are no vault:// refs, OpenBao is not contacted.
 */
class JsonConfigParser {
   public:
    /**
     * Load and validate config from a JSON file.
     * @param file_path Path to the JSON file (e.g. config/das.json).
     * @return JsonConfig instance with validated schema (vault refs resolved).
     * @param throw_flag Whether to throw an exception on file not found.
     */
    static JsonConfig load(const string& file_path, bool throw_flag = true);

    /**
     * Load and validate config from a JSON string (e.g. for tests).
     * @param json_content Raw JSON string.
     * @return JsonConfig instance with validated schema (vault refs resolved).
     * @throws std::runtime_error on invalid JSON or schema validation failure.
     */
    static JsonConfig load_from_string(const string& json_content);
};

}  // namespace commons
