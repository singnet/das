#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "Properties.h"

using namespace std;

namespace commons {

/**
 * @brief Wrapper around a JSON value from at_path(); provides get_or<T>(default) when the path is
 * missing.
 */
class JsonPathValue {
   public:
    explicit JsonPathValue(nlohmann::json j) : j_(std::move(j)) {}

    /** Return true if the path was not found (null). */
    bool is_null() const { return j_.is_null(); }

    /** Get the value as T; throws if null or type mismatch. */
    template <typename T>
    T get() const {
        return j_.get<T>();
    }

    /** Get the value as T, or default_value if null or conversion fails. */
    template <typename T>
    T get_or(const T& default_value) const {
        if (j_.is_null()) return default_value;
        try {
            return j_.get<T>();
        } catch (...) {
            return default_value;
        }
    }

    /** Allow use as nlohmann::json (e.g. .is_string(), .dump()). */
    const nlohmann::json& operator*() const { return j_; }
    const nlohmann::json* operator->() const { return &j_; }

   private:
    nlohmann::json j_;
};

/**
 * @brief Holds a parsed JSON config (e.g. DAS config.json) with schema version
 * and optional nested Properties view. Inherits nlohmann::json so you can use
 * the full JSON API: dump(), items(), operator[], .get<T>(), etc.
 *
 * Example:
 *   JsonConfig config = JsonConfigParser::load(path);
 *   cout << config.dump() << endl;
 *   for (const auto& kv : config.items()) {
 *     cout << kv.key() << " = " << kv.value().dump() << endl;
 *   }
 *   auto ep = config["agents"]["query"]["endpoint"].get<std::string>();
 */
class JsonConfig : public nlohmann::json {
   public:
    /** Construct from already-parsed JSON. */
    JsonConfig(nlohmann::json root);

    JsonConfig();

    /** @return Schema version string (e.g. "2.0"). */
    string get_schema_version() const;

    /** @return The root JSON (same as *this; for API compatibility). */
    const nlohmann::json& get_json() const { return *this; }

    /**
     * Get a nested value by dotted path (e.g. "brokers.attention.endpoint").
     * @return JsonPathValue at that path; use .get<T>() or .get_or<T>(default) on the result.
     * Example: config.at_path("atomdb.type").get_or<string>("NOT_FOUND");
     */
    JsonPathValue at_path(const string& dotted_path) const;

    /**
     * @return Properties with nested structure: objects become inner Properties
     * (e.g. props.get_nested("atomdb")->get_nested("redis")->get<long>("port")).
     * Arrays become nested Properties with keys "0", "1", "2", ...
     */
    Properties to_properties() const;
};

/** Allows nlohmann::json::get<JsonConfig>() and get_or<JsonConfig>(). */
inline void from_json(const nlohmann::json& j, JsonConfig& config) { config = JsonConfig(j); }

inline void to_json(nlohmann::json& j, const JsonConfig& config) {
    j = static_cast<const nlohmann::json&>(config);
}

}  // namespace commons
