#include "ProxyParametersFromJson.h"

#include <algorithm>
#include <cctype>
#include <variant>

using namespace command_router;
using namespace commons;

bool ProxyParametersFromJson::set_bool(PropertyValue& current,
                                       const json& value,
                                       const string& key,
                                       string& error_message) {
    if (value.is_boolean()) {
        current = value.get<bool>();
        return true;
    }
    if (value.is_string()) {
        const string& text = value.get_ref<const string&>();
        if (text == "true" || text == "1") {
            current = true;
            return true;
        }
        if (text == "false" || text == "0") {
            current = false;
            return true;
        }
    }
    if (value.is_number_integer()) {
        const long long number = value.get<long long>();
        if (number == 0 || number == 1) {
            current = (number == 1);
            return true;
        }
    }
    error_message = "Parameter '" + key + "' expects bool (true, false, 1, or 0)";
    return false;
}

bool ProxyParametersFromJson::set_unsigned_int(PropertyValue& current,
                                               const json& value,
                                               const string& key,
                                               string& error_message) {
    const string uint_error = "Parameter '" + key + "' expects unsigned integer";
    const auto fits_uint = [](unsigned long long number) {
        return static_cast<unsigned int>(number) == number;
    };

    if (value.is_number_unsigned()) {
        const unsigned long long number = value.get<unsigned long long>();
        if (!fits_uint(number)) {
            error_message = uint_error;
            return false;
        }
        current = static_cast<unsigned int>(number);
        return true;
    }
    if (value.is_number_integer()) {
        const long long number = value.get<long long>();
        if (number < 0 || !fits_uint(static_cast<unsigned long long>(number))) {
            error_message = uint_error;
            return false;
        }
        current = static_cast<unsigned int>(number);
        return true;
    }
    if (value.is_string()) {
        const string& text = value.get_ref<const string&>();
        const bool all_digits = !text.empty() && all_of(text.begin(), text.end(), [](unsigned char c) {
            return isdigit(c);
        });
        if (!all_digits) {
            error_message = uint_error;
            return false;
        }
        try {
            size_t consumed = 0;
            const unsigned long long parsed = stoull(text, &consumed);
            if (consumed != text.size() || !fits_uint(parsed)) {
                error_message = uint_error;
                return false;
            }
            current = static_cast<unsigned int>(parsed);
            return true;
        } catch (const exception&) {
            error_message = uint_error;
            return false;
        }
    }
    error_message = uint_error;
    return false;
}

bool ProxyParametersFromJson::set_long(PropertyValue& current,
                                       const json& value,
                                       const string& key,
                                       string& error_message) {
    if (value.is_number_integer()) {
        current = static_cast<long>(value.get<long long>());
        return true;
    }
    if (value.is_string()) {
        try {
            size_t consumed = 0;
            const long parsed = stol(value.get_ref<const string&>(), &consumed);
            if (consumed != value.get_ref<const string&>().size()) {
                error_message = "Parameter '" + key + "' expects integer";
                return false;
            }
            current = parsed;
            return true;
        } catch (const exception&) {
            error_message = "Parameter '" + key + "' expects integer";
            return false;
        }
    }
    error_message = "Parameter '" + key + "' expects integer";
    return false;
}

bool ProxyParametersFromJson::set_double(PropertyValue& current,
                                         const json& value,
                                         const string& key,
                                         string& error_message) {
    if (value.is_number()) {
        current = value.get<double>();
        return true;
    }
    if (value.is_string()) {
        try {
            size_t consumed = 0;
            const double parsed = stod(value.get_ref<const string&>(), &consumed);
            if (consumed != value.get_ref<const string&>().size()) {
                error_message = "Parameter '" + key + "' expects number";
                return false;
            }
            current = parsed;
            return true;
        } catch (const exception&) {
            error_message = "Parameter '" + key + "' expects number";
            return false;
        }
    }
    error_message = "Parameter '" + key + "' expects number";
    return false;
}

bool ProxyParametersFromJson::set_string(PropertyValue& current,
                                         const json& value,
                                         const string& key,
                                         string& error_message) {
    if (!value.is_string()) {
        error_message = "Parameter '" + key + "' expects string";
        return false;
    }
    const string& text = value.get_ref<const string&>();
    if (text.empty()) {
        error_message = "Parameter '" + key + "' expects non-empty string";
        return false;
    }
    current = text;
    return true;
}

bool ProxyParametersFromJson::set(Properties& proxy_parameters,
                                  const json& params,
                                  const string& command,
                                  string& error_message) {
    for (const auto& [key, value] : params.items()) {
        if (key == command) {
            continue;
        }

        auto param_it = proxy_parameters.find(key);
        if (param_it == proxy_parameters.end()) {
            error_message = "Unknown parameter: '" + key + "'";
            return false;
        }

        PropertyValue& current = param_it->second;
        if (holds_alternative<bool>(current)) {
            if (!set_bool(current, value, key, error_message)) {
                return false;
            }
        } else if (holds_alternative<unsigned int>(current)) {
            if (!set_unsigned_int(current, value, key, error_message)) {
                return false;
            }
        } else if (holds_alternative<long>(current)) {
            if (!set_long(current, value, key, error_message)) {
                return false;
            }
        } else if (holds_alternative<double>(current)) {
            if (!set_double(current, value, key, error_message)) {
                return false;
            }
        } else if (holds_alternative<string>(current)) {
            if (!set_string(current, value, key, error_message)) {
                return false;
            }
        } else {
            error_message = "Parameter '" + key + "' has unsupported type";
            return false;
        }
    }
    return true;
}
