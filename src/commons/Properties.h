#pragma once

namespace commons {

using PropertyValue = variant<string, long, double, bool>;

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
    const T* get(const string& key) const {
        auto it = this->find(key);
        if (it != this->end()) {
            if (auto attr = std::get_if<T>(&it->second)) {
                return attr;
            }
        }
        return nullptr;
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
            } else if (auto integer = std::get_if<long>(&value)) {
                result += std::to_string(*integer);
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
};

}  // namespace commons
