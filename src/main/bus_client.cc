#include <signal.h>

#include "FitnessFunctionRegistry.h"
#include "Helper.h"
#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "Logger.h"
#include "Properties.h"
#include "ProxyFactory.h"
#include "RemoteAtomDB.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace commons;
using namespace mains;
using namespace std;

int main(int argc, char* argv[]) {
    try {
        auto required_cmd_args = {
            Helper::SERVICE, Helper::ENDPOINT, Helper::BUS_ENDPOINT, Helper::PORTS_RANGE};
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
            Helper::get_required_arguments(cmd_args[Helper::SERVICE], ServiceCallerType::CLIENT);
        for (auto req_arg : required_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                Utils::error("Required argument missing: " + string(req_arg));
            }
        }

        ///////// Loading JSON config
        auto it_config = cmd_args.find("config");
        JsonConfig json_config;
        if (it_config != cmd_args.end() && !it_config->second.empty()) {
            json_config = JsonConfigParser::load(it_config->second);
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

        LOG_INFO("Starting service: " + cmd_args[Helper::SERVICE]);
        auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
        AtomDBSingleton::init(atomdb_config);

        if (Helper::processor_type_from_string(cmd_args[Helper::SERVICE]) ==
            mains::ProcessorType::EVOLUTION_AGENT) {
            LOG_INFO("Initializing Fitness Function Registry statics...");
            FitnessFunctionRegistry::initialize_statics();
        }
        auto proxy = ProxyFactory::create_proxy(cmd_args[Helper::SERVICE], props);
        if (proxy == nullptr) {
            Utils::error("Could not create proxy for service or client is inactive: " +
                         cmd_args[Helper::SERVICE]);
        }
        auto ports_range = Utils::parse_ports_range(props.get<string>(Helper::PORTS_RANGE));
        ServiceBusSingleton::init(props.get<string>(Helper::ENDPOINT),
                                  props.get<string>(Helper::BUS_ENDPOINT),
                                  ports_range.first,
                                  ports_range.second);
        shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
        service_bus->issue_bus_command(proxy);

        if (cmd_args[Helper::SERVICE] == "atomdb-broker") {
            auto action = props.get_or<string>("action", "");
            auto tokens_str = props.get_or<string>("tokens", "");
            vector<string> tokens = Utils::split(tokens_str, ' ');
            shared_ptr<atomdb_broker::AtomDBProxy> atomdb_proxy =
                dynamic_pointer_cast<atomdb_broker::AtomDBProxy>(proxy);
            if (action == atomdb_broker::AtomDBProxy::ADD_ATOMS) {
                vector<string> response = atomdb_proxy->add_atoms(tokens, false);
                if (response.empty()) {
                    cout << "No answers" << endl;
                } else {
                    for (auto resp : response) {
                        cout << "handle: " << resp << endl;
                    }
                }
            }
            return 0;
        }

        // Default case for other clients
        while (proxy->finished() == false && Helper::is_running) {
            if (dynamic_cast<BaseQueryProxy*>(proxy.get()) != nullptr) {
                auto query_proxy = dynamic_cast<BaseQueryProxy*>(proxy.get());
                shared_ptr<QueryAnswer> answer;
                while ((answer = query_proxy->pop()) != nullptr) {
                    LOG_INFO("Received answer: " + answer->to_string());
                    for (string handle : answer->get_handles_vector()) {
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
