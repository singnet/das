#pragma once

#include <memory>
#include <string>

#include "BusCommandRouterProxy.h"
#include "nlohmann/json.hpp"

using namespace std;

using json = nlohmann::json;

namespace command_router {

/**
 * @brief Builds a BusCommandRouterProxy from an HTTP {command, params} request.
 *
 * Parses the command-specific structural fields into the bus ARG string, constructs
 * the proxy with defaults, then overlays remaining scalar params onto proxy->parameters.
 */
class HttpCommandProxyFactory {
   public:
    /** Known HTTP command names. */
    static constexpr const char* QUERY = "query";

    /**
     * @brief Create a dispatch-ready proxy for the given HTTP command.
     * @return Ready proxy on success; nullptr with error_message on failure.
     */
    static shared_ptr<BusCommandRouterProxy> create(const string& command,
                                                    const json& params,
                                                    string& error_message);
};

}  // namespace command_router
