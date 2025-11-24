#include <signal.h>

#include "AttentionBrokerClient.h"
#include "FitnessFunctionRegistry.h"
#include "Helper.h"
#include "Logger.h"
#include "ProcessorFactory.h"
#include "Properties.h"
#include "Utils.h"

using namespace commons;
using namespace mains;
using namespace std;
using namespace attention_broker;

void ctrl_c_handler(int) {
    LOG_INFO("Stopping service...");
    LOG_INFO("Done.");
    exit(0);
}

int main(int argc, char* argv[]) {
    try {
        auto required_cmd_args = {Helper::SERVICE, Helper::ENDPOINT, Helper::PORTS_RANGE};
        auto cmd_args = Utils::parse_command_line(argc, argv);
        ///////// Checking args
        if (cmd_args.find("help") != cmd_args.end()) {
            cout << Helper::help(Helper::processor_type_from_string(cmd_args[Helper::SERVICE])) << endl;
            return 0;
        }
        for (auto req_arg : required_cmd_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                Utils::error("Required argument missing: " + string(req_arg));
            }
        }
        auto required_args = Helper::get_required_arguments(cmd_args[Helper::SERVICE]);
        for (auto req_arg : required_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                Utils::error("Required argument missing: " + string(req_arg));
            }
        }
        ///////// Handling signals
        signal(SIGINT, ctrl_c_handler);
        signal(SIGTERM, ctrl_c_handler);
        ///////// Initializing dependencies
        LOG_INFO("Starting service: " + cmd_args[Helper::SERVICE]);
        auto props = Properties(cmd_args.begin(), cmd_args.end());
        if (props.find(Helper::ATTENTION_BROKER_ADDRESS) != props.end()) {
            AttentionBrokerClient::set_server_address(
                props.get<string>(Helper::ATTENTION_BROKER_ADDRESS));
        }
        if (props.get_or<string>(Helper::USE_MORK, "false") == "true") {
            AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
        } else {
            AtomDBSingleton::init();
        }

        if (Helper::processor_type_from_string(cmd_args[Helper::SERVICE]) ==
                mains::ProcessorType::INFERENCE_AGENT ||
            Helper::processor_type_from_string(cmd_args[Helper::SERVICE]) ==
                mains::ProcessorType::EVOLUTION_AGENT) {
            fitness_functions::FitnessFunctionRegistry::initialize_statics();
        }

        auto ports_range = Utils::parse_ports_range(props.get<string>(Helper::PORTS_RANGE));
        ServiceBusSingleton::init(props.get<string>(Helper::ENDPOINT),
                                  props.get_or<string>(Helper::BUS_ENDPOINT, ""),
                                  ports_range.first,
                                  ports_range.second);
        ///////// Registering processor
        auto service = ProcessorFactory::create_processor(cmd_args[Helper::SERVICE], props);
        if (service == nullptr) {
            Utils::error("Could not create processor for service or service is inactive: " +
                         cmd_args[Helper::SERVICE]);
        }

        LOG_INFO("Registering processor for service: " + cmd_args[Helper::SERVICE]);

        shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
        LOG_INFO("Processor registered for service: " + cmd_args[Helper::SERVICE]);
        service_bus->register_processor(service);

        while (true) {
            Utils::sleep();
        }

    } catch (const std::exception& e) {
        return 1;
    }
    return 0;
}
