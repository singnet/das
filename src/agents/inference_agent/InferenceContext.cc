#include "InferenceContext.h"

#include "ContextBrokerProxy.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace std;
using namespace inference_agent;
using namespace service_bus;
using namespace context_broker;
// clang-format off
// TODO Hardcoded for tunning --remove--
static vector<string> context_query_vec = {
        // "OR", "3",
        // "LINK_TEMPLATE", "Expression", "3",
        //     "NODE", "Symbol", "Evaluation",
        //     "VARIABLE", "V2",
        //     "VARIABLE", "V3",
        // "LINK_TEMPLATE", "Expression", "3",
        //     "NODE", "Symbol", "Contains",
        //     "VARIABLE", "V2",
        //     "VARIABLE", "V3",
        // "LINK_TEMPLATE", "Expression", "2",
        //     "NODE", "Symbol", "Concept",
        //     "VARIABLE", "V2",
    "OR", "2",
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Evaluation",
            "VARIABLE", "V2",
            "VARIABLE", "V3",
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Contains",
            "VARIABLE", "V2",
            "VARIABLE", "V3"

    };

static string determiners_str = "_0:$V2,_0:$V3";

static bool use_cache = false;
static bool enforce_cache_recreation = false;
static double initial_rent_rate = 0.25;
static double initial_spreading_rate_lowerbound = 0.90;
static double initial_spreading_rate_upperbound = 0.90;
// clang-format on

InferenceContext::InferenceContext(const string& name,
                                   const string& query,
                                   const string& determiner_schema,
                                   const string& stimulus_schema) {
    auto proxy = make_shared<ContextBrokerProxy>(name, context_query_vec, determiners_str, "");
    proxy->parameters[ContextBrokerProxy::USE_CACHE] = use_cache;
    proxy->parameters[ContextBrokerProxy::ENFORCE_CACHE_RECREATION] = enforce_cache_recreation;
    proxy->parameters[ContextBrokerProxy::INITIAL_RENT_RATE] = initial_rent_rate;
    proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_LOWERBOUND] =
        initial_spreading_rate_lowerbound;
    proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_UPPERBOUND] =
        initial_spreading_rate_upperbound;
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    this->context_name = proxy->get_key();
    while (!proxy->is_context_created()) {
        Utils::sleep(200);
    }
    // this->context_name = name;
}

InferenceContext::~InferenceContext() {}

string InferenceContext::get_context_name() const { return this->context_name; }
