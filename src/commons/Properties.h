#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Utils.h"

using namespace std;

namespace commons {

using PropertyValue = variant<string, long, unsigned int, double, bool>;

/**
 * @brief A specialization of std::unordered_map using string as key and
 * PropertyValue as value, with additional helper methods.
 *
 * This class specializes std::unordered_map to use string as the key type and
 * PropertyValue as the value type. It also implements several helper methods for
 * convenient access and manipulation of custom attributes.
 */
class Properties : public unordered_map<string, PropertyValue> {
   public:
    using unordered_map<string,
                        PropertyValue>::unordered_map;  // Inherit constructors

    /**
     * @brief Construct a Properties from an initializer list.
     * @param init The initializer list of key-value pairs.
     * @example
     * Properties attrs = {{"key1", "value1"}, {"key2", 42}};
     */
    template <typename K, typename V>
    Properties(std::initializer_list<std::pair<K, V>> init)
        : unordered_map<string, PropertyValue>(init.begin(), init.end()) {}

    /**
     * @brief Get the value associated with a key, cast to the requested type.
     *
     * If the key exists and the value can be cast to type T, returns a pointer to the value.
     * Otherwise, returns nullptr.
     *
     * @tparam T The type to cast the value to (must match the variant type).
     * @param key The key to look up in the map.
     * @return Pointer to the value if present and of the correct type, otherwise nullptr.
     */
    template <typename T>
    const T* get_ptr(const string& key) const {
        auto it = this->find(key);
        if (it != this->end()) {
            if (auto attr = std::get_if<T>(&it->second)) {
                return attr;
            }
        }
        return nullptr;
    }

    /**
     * @brief Get the value associated with a key, cast to the requested type.
     *
     * If the key exists and the value can be cast to type T, returns the value.
     * Otherwise, raises an error.
     *
     * @tparam T The type to cast the value to (must match the variant type).
     * @param key The key to look up in the map.
     * @return The value if present and of the correct type, otherwise raises an error.
     */
    template <typename T>
    const T get(const string& key) const {
        auto it = this->find(key);
        if (it != this->end()) {
            if (auto attr = std::get_if<T>(&it->second)) {
                return *attr;
            }
        }
        Utils::error("Unkown property key: " + key);
    }
    /**
     * @brief Convert the Properties to a string representation.
     * @return A string representation of the map in the form {key1: value1, key2: value2, ...}.
     */
    string to_string() const {
        string result = "{";
        if (this->empty()) {
            return result + "}";
        }

        // Sort keys for consistent output
        // Note: This is not strictly necessary, but it makes the output predictable and easier to read.
        vector<string> keys;
        for (const auto& pair : *this) {
            keys.push_back(pair.first);
        }
        std::sort(keys.begin(), keys.end());

        for (const auto& key : keys) {
            const auto& value = this->at(key);
            result += key + ": ";
            if (auto str = std::get_if<string>(&value)) {
                result += "'" + *str + "'";
            } else if (auto longint = std::get_if<long>(&value)) {
                result += std::to_string(*longint);
            } else if (auto uinteger = std::get_if<unsigned int>(&value)) {
                result += std::to_string(*uinteger);
            } else if (auto floating = std::get_if<double>(&value)) {
                result += std::to_string(*floating);
            } else if (auto boolean = std::get_if<bool>(&value)) {
                result += *boolean ? "true" : "false";
            }
            result += ", ";
        }

        // Remove the last comma and space
        result.pop_back();
        result.pop_back();

        return result + "}";
    }

    /**
     * @brief Convert the Properties to a representation as a vector of tokens (strings)
     * @return A string representation of the map in the form {key1, type1, value1, key2, type2, value2,
     * ...}.
     */
    vector<string> tokenize() const {
        vector<string> result;
        if (!this->empty()) {
            // Sort keys for consistent output
            // Note: This is not strictly necessary, but it makes the output predictable and easier to
            // read.
            vector<string> keys;
            for (const auto& pair : *this) {
                keys.push_back(pair.first);
            }
            std::sort(keys.begin(), keys.end());

            for (const auto& key : keys) {
                result.push_back(key);
                const auto& value = this->at(key);
                if (auto str = std::get_if<string>(&value)) {
                    result.push_back("string");
                    result.push_back(*str);
                } else if (auto longint = std::get_if<long>(&value)) {
                    result.push_back("long");
                    result.push_back(std::to_string(*longint));
                } else if (auto uinteger = std::get_if<unsigned int>(&value)) {
                    result.push_back("unsigned_int");
                    result.push_back(std::to_string(*uinteger));
                } else if (auto floating = std::get_if<double>(&value)) {
                    result.push_back("double");
                    result.push_back(std::to_string(*floating));
                } else if (auto boolean = std::get_if<bool>(&value)) {
                    result.push_back("bool");
                    result.push_back(*boolean ? "true" : "false");
                }
            }
        }
        return result;
    }

    /**
     * @brief Build a Properties object based on a vector of string tokens like
     * {key1, type1, value1, key2, type2, value2, ...}.
     *
     * @return The Properties object with keys and values as defined in the tokens.
     */
    void untokenize(const vector<string>& tokens) {
        if ((tokens.size() % 3) == 0) {
            unsigned int cursor = 0;
            while (cursor != tokens.size()) {
                string key = tokens[cursor++];
                string type = tokens[cursor++];
                string value = tokens[cursor++];
                if (type == "string") {
                    (*this)[key] = value;
                } else if (type == "long") {
                    (*this)[key] = (long) stoi(value);
                } else if (type == "unsigned_int") {
                    (*this)[key] = (unsigned int) stoi(value);
                } else if (type == "double") {
                    (*this)[key] = stod(value);
                } else if (type == "bool") {
                    if (value == "true") {
                        (*this)[key] = true;
                    } else if (value == "false") {
                        (*this)[key] = false;
                    } else {
                        Utils::error("Invalid 'bool' string value: " + value);
                    }
                } else {
                    Utils::error("Invalid token type: " + type);
                }
            }
        } else {
            Utils::error("Invalid tokens vector size: " + std::to_string(tokens.size()));
        }
    }
};

}  // namespace commons
