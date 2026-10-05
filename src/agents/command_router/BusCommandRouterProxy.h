#pragma once

#include <mutex>
#include <vector>

#include "BaseQueryProxy.h"

using namespace std;
using namespace service_bus;
using namespace agents;

namespace command_router {

/**
 * Proxy for the BUS_COMMAND_ROUTER service.
 *
 * Wire format: proxy->args = {COMMAND, ARG} (two strings).
 * Extends BaseQueryProxy so query/evolution answers from downstream services are received
 * directly on the client's proxy (no router relay).
 *
 * For HTTP evolution with remote fitness, EVAL_FITNESS from the evolution agent is buffered
 * here so the stream poller can forward it over WebSocket and reply with EVAL_FITNESS_RESPONSE.
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

    /**
     * @brief If an EVAL_FITNESS request is pending, move its tokenized answers into @p out.
     * @return true when a request was taken.
     */
    bool take_pending_eval_fitness(vector<string>& out);

    /** @brief Forward fitness floats to the evolution agent as EVAL_FITNESS_RESPONSE. */
    void send_eval_fitness_response(const vector<string>& fitness_values);

    string params_response;
    string set_param_ack;
    bool routed_flag;
    bool count_received = false;

   private:
    void count_answer(const vector<string>& args);

    mutex api_mutex;
    mutex fitness_mutex_;
    bool pending_eval_fitness_ = false;
    vector<string> pending_eval_fitness_args_;
};

}  // namespace command_router
