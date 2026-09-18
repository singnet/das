#include "BusCommandRouterProxy.h"

#include "BaseQueryProxy.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"
#include "SystemParametersSingleton.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace command_router;
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
    parameters += SystemParametersSingleton::get_instance()->get_command_router_params();
}

void BusCommandRouterProxy::pack_command_line_args() {
    if (this->args.size() < 2) {
        RAISE_ERROR("BusCommandRouterProxy requires args {COMMAND, ARG}");
    }
}

bool BusCommandRouterProxy::from_remote_peer(const string& command, const vector<string>& args) {
    if (command == QueryEvolutionProxy::EVAL_FITNESS) {
        lock_guard<mutex> fitness_lock(this->fitness_mutex_);
        this->pending_eval_fitness_args_ = args;
        this->pending_eval_fitness_ = true;
        return true;
    }

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
    } else if (command == PatternMatchingQueryProxy::COUNT) {
        count_answer(args);
        return true;
    }
    return BaseQueryProxy::from_remote_peer(command, args);
}

bool BusCommandRouterProxy::take_pending_eval_fitness(vector<string>& out) {
    lock_guard<mutex> fitness_lock(this->fitness_mutex_);
    if (!this->pending_eval_fitness_) {
        return false;
    }
    out = std::move(this->pending_eval_fitness_args_);
    this->pending_eval_fitness_args_.clear();
    this->pending_eval_fitness_ = false;
    return true;
}

void BusCommandRouterProxy::send_eval_fitness_response(const vector<string>& fitness_values) {
    to_remote_peer(QueryEvolutionProxy::EVAL_FITNESS_RESPONSE, fitness_values);
}

void BusCommandRouterProxy::count_answer(const vector<string>& args) {
    if (this->is_aborting()) {
        return;
    }
    if (args.size() != 1) {
        RAISE_ERROR("Invalid args for count command");
    }
    const int parsed = stoi(args[0]);
    if (parsed < 0) {
        RAISE_ERROR("Invalid count value");
    }
    this->set_count(static_cast<unsigned int>(parsed));
    this->count_received = true;
}
