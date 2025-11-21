#include <signal.h>

#include "Logger.h"
#include "Properties.h"
#include "ProxyFactory.h"
#include "Helper.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace commons;
using namespace mains;
using namespace std;

int main(int argc, char* argv[]) {
    try {
        auto required_cmd_args = {Helper::SERVICE,
                                  Helper::HOSTNAME,
                                  Helper::SERVICE_HOSTNAME,
                                  Helper::PORTS_RANGE};
        auto cmd_args = Utils::parse_command_line(argc, argv);
        if (cmd_args.find("help") != cmd_args.end()) {
            cout << Helper::help(Helper::processor_type_from_string(cmd_args["service"]),
                                       ServiceCallerType::CLIENT)
                 << endl;
            return 0;
        }
        for (auto req_arg : required_cmd_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                Utils::error("Required argument missing: " + string(req_arg));
            }
        }

        auto required_args =
            Helper::get_required_arguments(cmd_args["service"], ServiceCallerType::CLIENT);
        for (auto req_arg : required_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                Utils::error("Required argument missing: " + string(req_arg));
            }
        }
        auto props = Properties(cmd_args.begin(), cmd_args.end());
        LOG_INFO("Parsed Properties: " + props.to_string());
        auto signal_handler = [](int) {
            LOG_INFO("Stopping client...");
            Helper::is_running = false;
            LOG_INFO("Done.");
            exit(0);
        };
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        LOG_INFO("Starting service: " + cmd_args["service"]);
        if (props.get_or<string>("use-mork", "false") == "true") {
            AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
        } else {
            AtomDBSingleton::init();
        }
        auto proxy = ProxyFactory::create_proxy(cmd_args["service"], props);
        if (proxy == nullptr) {
            Utils::error("Could not create proxy for service or client is inactive: " +
                         cmd_args["service"]);
        }
        auto ports_range = Utils::parse_ports_range(props.get<string>("ports-range"));
        ServiceBusSingleton::init(props.get<string>("hostname"),
                                  props.get<string>("service-hostname"),
                                  ports_range.first,
                                  ports_range.second);
        shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
        service_bus->issue_bus_command(proxy);

        // Special case for AtomDBBroker client
        // Note to reviewer: The original client doesn't work, this also doesn't work but maintains
        // the original structure. Proper fix will be done later.
        // if (cmd_args["service"] == "atomdb-broker") {
        //     auto action = props.get_or<string>("action", "");
        //     auto tokens_str = props.get_or<string>("tokens", "");
        //     vector<string> tokens = Utils::split(tokens_str, ' ');
        //     shared_ptr<atomdb_broker::AtomDBProxy> atomdb_proxy =
        //         dynamic_pointer_cast<atomdb_broker::AtomDBProxy>(proxy);
        //     if (action == atomdb_broker::AtomDBProxy::ADD_ATOMS) {
        //         auto atoms = atomdb_proxy->build_atoms_from_tokens(tokens);
        //         vector<string> response = atomdb_proxy->add_atoms(atoms);
        //         if (response.empty()) {
        //             cout << "No answers" << endl;
        //         } else {
        //             for (auto resp : response) {
        //                 cout << "handle: " << resp << endl;
        //             }
        //         }
        //     }
        //     return 0;
        // }

        // Default case for other clients
        while (proxy->finished() == false && Helper::is_running) {
            if (dynamic_cast<BaseQueryProxy*>(proxy.get()) != nullptr) {
                auto query_proxy = dynamic_cast<BaseQueryProxy*>(proxy.get());
                shared_ptr<QueryAnswer> answer;
                while ((answer = query_proxy->pop()) != nullptr) {
                    LOG_INFO("Received answer: " + answer->to_string());
                    for (string handle : answer->handles) {
                        LOG_INFO(answer->metta_expression[handle]);
                    }
                }
            }
            Utils::sleep(100);
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Exception in bus_client: " + string(e.what()));
        return 1;
    }
    return 0;
}