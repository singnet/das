#pragma once

#include <optional>
#include <unordered_map>
#include <variant>

#include "Utils.h"

using namespace std;
using namespace commons;

namespace atomdb {
namespace atomdb_api_types {

// -------------------------------------------------------------------------------------------------
// NOTE TO REVIEWER:
//
// This class will be replaced/integrated by/with classes already implemented in das-atom-db.
//
// However, that classes will need to be revisited in order to allow the methods implemented here
// because although the design of such methods is nasty, they have the string advantage of
// allowing the reuse of structures allocated by the DBMS (Redis an MongoDB) without the need
// of re-allocation of other dataclasses. Although this nasty behavior may not be desirable
// outside the DAS bounds, it's quite appealing inside the query engine (and perhaps other
// parts of internal stuff).
//
// I think it's pointless to make any further documentation while we don't make this integration.
// -------------------------------------------------------------------------------------------------

using CustomAttributesValue = variant<string, long, double, bool>;

/**
 * @brief A specialization of std::unordered_map using string as key and
 * CustomAttributesValue as value, with additional helper methods.
 *
 * This class specializes std::unordered_map to use string as the key type and
 * CustomAttributesValue as the value type. It also implements several helper methods for
 * convenient access and manipulation of custom attributes.
 */
class CustomAttributesMap : public unordered_map<string, CustomAttributesValue> {
   public:
    using unordered_map<string,
                        CustomAttributesValue>::unordered_map;  // Inherit constructors

    /**
     * @brief Get the value associated with a key, cast to the requested type.
     *
     * If the key exists and the value can be cast to type T, returns an optional containing the value.
     * Otherwise, returns std::nullopt.
     *
     * @tparam T The type to cast the value to (must match the variant type).
     * @param key The key to look up in the map.
     * @return std::optional<T> containing the value if present and of the correct type, otherwise
     * std::nullopt.
     */
    template <typename T>
    const optional<T> get(const string& key) const {
        auto it = this->find(key);
        if (it != this->end()) {
            if (auto attr = std::get_if<T>(&it->second)) {
                return *attr;
            }
        }
        return nullopt;
    }

    /**
     * @brief Convert the CustomAttributesMap to a string representation.
     * @return A string representation of the map in the form {key1: value1, key2: value2, ...}.
     */
    string to_string() const {
        string result = "{";
        for (const auto& [key, value] : *this) {
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
        result.pop_back();
        result.pop_back();
        result += "}";
        return result;
    }
};

class HandleList {
   public:
    HandleList() {}
    virtual ~HandleList() {}

    virtual const char* get_handle(unsigned int index) = 0;
    virtual unsigned int size() = 0;
};

class HandleSetIterator {
   public:
    virtual char* next() = 0;
};

class HandleSet {
   public:
    HandleSet() {}
    virtual ~HandleSet() {}

    virtual unsigned int size() = 0;
    virtual void append(shared_ptr<HandleSet> other) = 0;
    virtual shared_ptr<HandleSetIterator> get_iterator() = 0;
};

class AtomDocument {
   public:
    AtomDocument() {}
    virtual ~AtomDocument() {}

    virtual const char* get(const string& key) = 0;
    virtual const char* get(const string& array_key, unsigned int index) = 0;
    virtual unsigned int get_size(const string& array_key) = 0;
    virtual bool contains(const string& key) = 0;
};

}  // namespace atomdb_api_types
}  // namespace atomdb
