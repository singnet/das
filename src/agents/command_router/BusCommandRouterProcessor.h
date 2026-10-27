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

    /** Run a router command in-process for HTTP; responses are written to caller_proxy. */
    void dispatch_http_command(const shared_ptr<BusCommandRouterProxy>& caller_proxy,
                               const string& http_requestor_id);

    /**
     * Forward an HTTP query to the query engine. Answers return on caller_proxy.
     * Does not open a second proxy.
     */
    void dispatch_http_query(const shared_ptr<BusCommandRouterProxy>& caller_proxy,
                             const string& http_requestor_id);

    /**
     * Execute an atomdb command and return the result.
     */
    string execute_atomdb_cmd(const string& command, const string& arg);

   private:
    void run_command_internal(shared_ptr<BusCommandRouterProxy> router_proxy, bool load_peer_parameters);

    void handle_get(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_set(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_query(shared_ptr<BusCommandRouterProxy> proxy,
                      const string& arg,
                      const string& answer_node_id,
                      bool notify_routed);
    void handle_evolution(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_link_creation(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_add_atom(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_delete_atom(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    string add_atom(const string& arg);
    string delete_atom(const string& arg);

    void forward_to_service(shared_ptr<BusCommandRouterProxy> router_proxy,
                            shared_ptr<BaseQueryProxy> service_proxy,
                            const string& answer_node_id,
                            bool notify_routed);

    void prepare_http_caller_proxy(const shared_ptr<BusCommandRouterProxy>& caller_proxy,
                                   const string& http_requestor_id);

    void set_router_param(BusCommandRouterProxy* proxy, const string& key, const string& value);

    Properties& parameters_for_peer(const string& peer_id);

    /** Router params keyed by bus requestor_id (client ServiceBus host_id). */
    unordered_map<string, Properties> router_parameters_by_peer;
    mutex router_parameters_mutex;
    shared_ptr<ServiceBus> service_bus;
    shared_ptr<atomdb::AtomDB> atomdb;
};

}  // namespace command_router
