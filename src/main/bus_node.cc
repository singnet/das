#include <signal.h>

#include "Logger.h"
#include "ProcessorFactory.h"
#include "Properties.h"
#include "RunnerHelper.h"
#include "AttentionBrokerClient.h"
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
        auto required_cmd_args = {"service", "hostname", "ports_range"};
        auto cmd_args = Utils::parse_command_line(argc, argv);
        if (cmd_args.find("help") != cmd_args.end()) {
            cout << RunnerHelper::help(RunnerHelper::processor_type_from_string(cmd_args["service"]))
                 << endl;
            return 0;
        }
        for (auto req_arg : required_cmd_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                Utils::error("Required argument missing: " + string(req_arg));
            }
        }
        auto required_args = RunnerHelper::get_required_arguments(cmd_args["service"]);
        for (auto req_arg : required_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                Utils::error("Required argument missing: " + string(req_arg));
            }
        }
        signal(SIGINT, ctrl_c_handler);
        signal(SIGTERM, ctrl_c_handler);
        LOG_INFO("Starting service: " + cmd_args["service"]);
        auto props = Properties(cmd_args.begin(), cmd_args.end());
        if (props.get_or<string>("use_mork", "false") == "true") {
            AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
        } else {
            AtomDBSingleton::init();
        }
        if (props.find("attention_broker_address") != props.end()) {
            AttentionBrokerClient::set_server_address(
                props.get<string>("attention_broker_address"));
        }
        auto service = ProcessorFactory::create_processor(cmd_args["service"], props);
        auto ports_range = Utils::parse_ports_range(props.get<string>("ports_range"));
        ServiceBusSingleton::init(props.get<string>("hostname"),
                                  props.get_or<string>("peer_address", ""),
                                  ports_range.first,
                                  ports_range.second);
        shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
        service_bus->register_processor(service);

        while (true) {
            Utils::sleep();
        }

    } catch (const std::exception& e) {
        return 1;
    }
    return 0;
}
