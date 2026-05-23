#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "BaseProxy.h"
#include "BusCommandProcessor.h"
#include "BusCommandRouterProxy.h"
#include "Properties.h"
#include "QueryAnswer.h"
#include "ServiceBus.h"

using namespace std;
using namespace service_bus;
using namespace agents;
using namespace query_engine;

namespace command_router {

class BusCommandRouterProcessor : public BusCommandProcessor {
   public:
    /**
     * @param service_bus Bus used to forward commands. If null, ServiceBusSingleton is used
     * (normal busnode deployment).
     */
    explicit BusCommandRouterProcessor(shared_ptr<ServiceBus> service_bus = nullptr);
    ~BusCommandRouterProcessor() override;

    shared_ptr<BusCommandProxy> factory_empty_proxy() override;
    void run_command(shared_ptr<BusCommandProxy> proxy) override;

   private:
    void dispatch(shared_ptr<BusCommandRouterProxy> proxy, const string& command, const string& arg);
    void handle_get(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_set(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_query(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_evolution(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);

    static void relay_query_answers_to_client(shared_ptr<BusCommandRouterProxy> client_proxy,
                                              shared_ptr<BaseQueryProxy> downstream);

    void set_router_param(BusCommandRouterProxy* proxy, const string& key, const string& value);
    void copy_query_parameters(shared_ptr<BaseProxy> target, shared_ptr<BusCommandRouterProxy> router);

    static string parse_metta_expression(const string& metta_expression);
    static vector<string> parse_request(const string& request_str, bool use_metta);
    static vector<vector<string>> parse_correlation_queries(const string& str, bool use_metta);
    static vector<map<string, QueryAnswerElement>> parse_correlation_replacements(const string& str,
                                                                                  bool use_metta);
    static vector<pair<QueryAnswerElement, QueryAnswerElement>> parse_correlation_mappings(
        const string& str, bool use_metta);

    Properties& parameters_for_peer(const string& peer_id);
    void load_router_parameters(shared_ptr<BusCommandRouterProxy> proxy);
    void save_router_parameters(shared_ptr<BusCommandRouterProxy> proxy);

    /** Router params keyed by bus requestor_id (client ServiceBus host_id). */
    unordered_map<string, Properties> router_parameters_by_peer;
    mutex router_parameters_mutex;
    shared_ptr<ServiceBus> service_bus;
};

}  // namespace command_router
