#pragma once

#include <string>

#include "Properties.h"
#include "nlohmann/json.hpp"

using namespace std;

using json = nlohmann::json;

namespace command_router {

class ProxyParametersFromJson {
   public:
    ProxyParametersFromJson() = delete;

    /**
     * @brief Sets the proxy parameters from the HTTP params JSON object.
     * @param proxy_parameters The proxy parameters to set.
     * @param params The HTTP params JSON object.
     * @param command The command name.
     * @param error_message The error message to set on failure.
     * @return True on success; false on failure.
     */
    static bool set(commons::Properties& proxy_parameters,
                    const json& params,
                    const string& command,
                    string& error_message);

   private:
    static bool set_bool(commons::PropertyValue& current,
                         const json& value,
                         const string& key,
                         string& error_message);
    static bool set_unsigned_int(commons::PropertyValue& current,
                                 const json& value,
                                 const string& key,
                                 string& error_message);
    static bool set_long(commons::PropertyValue& current,
                         const json& value,
                         const string& key,
                         string& error_message);
    static bool set_double(commons::PropertyValue& current,
                           const json& value,
                           const string& key,
                           string& error_message);
    static bool set_string(commons::PropertyValue& current,
                           const json& value,
                           const string& key,
                           string& error_message);
};

}  // namespace command_router
