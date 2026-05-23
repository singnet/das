#include "BusCommandRouterProxy.h"

#include "BaseQueryProxy.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace command_router;
using namespace query_engine;
using namespace evolution;

string BusCommandRouterProxy::PARAMS_RESPONSE = "params_response";
string BusCommandRouterProxy::SET_PARAM_ACK = "set_param_ack";
string BusCommandRouterProxy::ROUTED = "routed";

BusCommandRouterProxy::BusCommandRouterProxy() : BaseQueryProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::BUS_COMMAND_ROUTER;
    apply_default_parameters(this->parameters);
}

BusCommandRouterProxy::BusCommandRouterProxy(const string& router_command, const string& router_arg)
    : BaseQueryProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::BUS_COMMAND_ROUTER;
    this->args = {router_command, router_arg};
    apply_default_parameters(this->parameters);
}

BusCommandRouterProxy::~BusCommandRouterProxy() {}

void BusCommandRouterProxy::apply_default_parameters(Properties& parameters) {
    parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    parameters[BaseQueryProxy::MAX_BUNDLE_SIZE] = (unsigned int) 1000;
    parameters[BaseQueryProxy::MAX_ANSWERS] = (unsigned int) 100;
    parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = true;
    parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = true;
    parameters[BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH] = false;
    parameters["context"] = string("context");
    parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = false;
    parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = false;
    parameters[PatternMatchingQueryProxy::COUNT_FLAG] = false;
    parameters[QueryEvolutionProxy::POPULATION_SIZE] = (unsigned int) 100;
    parameters[QueryEvolutionProxy::MAX_GENERATIONS] = (unsigned int) 10;
    parameters[QueryEvolutionProxy::ELITISM_RATE] = (double) 0.08;
    parameters[QueryEvolutionProxy::SELECTION_RATE] = (double) 0.1;
}

void BusCommandRouterProxy::pack_command_line_args() {
    if (this->args.size() < 2) {
        RAISE_ERROR("BusCommandRouterProxy requires args {COMMAND, ARG}");
    }
}

bool BusCommandRouterProxy::from_remote_peer(const string& command, const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (command == PARAMS_RESPONSE) {
        if (!args.empty()) {
            this->params_response = args[0];
        }
        return true;
    } else if (command == SET_PARAM_ACK) {
        if (!args.empty()) {
            this->set_param_ack = args[0];
        }
        return true;
    } else if (command == ROUTED) {
        this->routed_flag = true;
        return true;
    }
    return BaseQueryProxy::from_remote_peer(command, args);
}
