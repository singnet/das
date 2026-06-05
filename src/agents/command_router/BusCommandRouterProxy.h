#pragma once

#include <mutex>

#include "BaseQueryProxy.h"

using namespace std;
using namespace service_bus;
using namespace agents;

namespace command_router {

/**
 * Proxy for the BUS_COMMAND_ROUTER service.
 *
 * Wire format: proxy->args = {COMMAND, ARG} (two strings).
 * Extends BaseQueryProxy so forwarded query/evolution answers can be received on the client.
 */
class BusCommandRouterProxy : public BaseQueryProxy {
   public:
    static string PARAMS_RESPONSE;
    static string SET_PARAM_ACK;
    static string ROUTED;

    BusCommandRouterProxy();
    BusCommandRouterProxy(const string& router_command, const string& router_arg);
    virtual ~BusCommandRouterProxy();

    /** Default router parameters; used by client proxies and the router processor store. */
    static void apply_default_parameters(Properties& parameters);

    virtual void pack_command_line_args() override;
    virtual bool from_remote_peer(const string& command, const vector<string>& args) override;

    string params_response;
    string set_param_ack;
    bool routed_flag;

   private:
    mutex api_mutex;
};

}  // namespace command_router
