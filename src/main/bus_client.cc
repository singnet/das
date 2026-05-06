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
            Helper::CLIENT, Helper::ENDPOINT, Helper::BUS_ENDPOINT, Helper::PORTS_RANGE};
        auto cmd_args = Utils::parse_command_line(argc, argv);
        if (cmd_args.find("help") != cmd_args.end()) {
            cout << Helper::help(Helper::processor_type_from_string(cmd_args[Helper::CLIENT]),
                                 ServiceCallerType::CLIENT)
                 << endl;
            return 0;
        }

        if (cmd_args.find(Helper::CLIENT) == cmd_args.end()) {
            Utils::error("Required argument missing: " + Helper::CLIENT);
        }

        ///////// Optional JsonConfig
        auto it_config = cmd_args.find("config");
        JsonConfig json_config;
        const bool has_client_config = it_config != cmd_args.end() && !it_config->second.empty();
        if (has_client_config) {
            json_config = JsonConfigParser::load_client_config(it_config->second);
            auto das_config_file = json_config.at_path("params.das-config-file");
            if (!das_config_file.is_null()) {
                JsonConfig das_config = JsonConfigParser::load(das_config_file.get<string>());
                // Merge atomdb config from das.json
                json_config["atomdb"] = das_config["atomdb"];

                // Get bus node endpoint from das.json
                auto it_known_peer = Helper::arg_to_json_config_key.find(cmd_args[Helper::CLIENT]);
                if (it_known_peer != Helper::arg_to_json_config_key.end()) {
                    cmd_args[Helper::BUS_ENDPOINT] =
                        das_config.at_path(it_known_peer->second + ".endpoint").get<string>();
                } else {
                    Utils::error("Required argument missing: " + cmd_args[Helper::CLIENT] +
                                 " is not a bus node.");
                }

            } else {
                Utils::error("params.das-config-file is missing");
            }

            cmd_args[Helper::ENDPOINT] = json_config.at_path("params.endpoint").get<string>();
            cmd_args[Helper::PORTS_RANGE] = json_config.at_path("params.ports-range").get<string>();

            // Merge all params from client json config to cmd_args, existing cmd_args have precedence
            Helper::merge_params_from_client_json_config(cmd_args, json_config);
        }

        for (auto req_arg : required_cmd_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                Utils::error("Required argument missing: " + string(req_arg));
            }
        }

        auto required_args =
            Helper::get_required_arguments(cmd_args[Helper::CLIENT], ServiceCallerType::CLIENT);
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

        LOG_INFO("Starting service: " + cmd_args[Helper::CLIENT]);
        auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
        AtomDBSingleton::init(atomdb_config);

        if (Helper::processor_type_from_string(cmd_args[Helper::CLIENT]) ==
            mains::ProcessorType::EVOLUTION_AGENT) {
            LOG_INFO("Initializing Fitness Function Registry statics...");
            FitnessFunctionRegistry::initialize_statics();
        }

        auto proxy = ProxyFactory::create_proxy(cmd_args[Helper::CLIENT], props);
        if (proxy == nullptr) {
            Utils::error("Could not create proxy for service or client is inactive: " +
                         cmd_args[Helper::CLIENT]);
        }
        auto ports_range = Utils::parse_ports_range(props.get<string>(Helper::PORTS_RANGE));
        ServiceBusSingleton::init(props.get<string>(Helper::ENDPOINT),
                                  props.get<string>(Helper::BUS_ENDPOINT),
                                  ports_range.first,
                                  ports_range.second);
        shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
        service_bus->issue_bus_command(proxy);

        LOG_DEBUG("ServiceBus host_id (endpoint): " + props.get<string>(Helper::ENDPOINT) +
                  "; known_peer (bus-endpoint): " + props.get<string>(Helper::BUS_ENDPOINT));

        if (cmd_args[Helper::CLIENT] == "atomdb-broker") {
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

        bool use_metta_as_query_tokens = cmd_args.find(Helper::USE_METTA_AS_QUERY_TOKENS) != cmd_args.end() && cmd_args[Helper::USE_METTA_AS_QUERY_TOKENS] == "true";

        // Default case for other clients
        while (proxy->finished() == false && Helper::is_running) {
            if (dynamic_cast<BaseQueryProxy*>(proxy.get()) != nullptr) {
                auto query_proxy = dynamic_cast<BaseQueryProxy*>(proxy.get());
                shared_ptr<QueryAnswer> answer;
                while ((answer = query_proxy->pop()) != nullptr) {
                    LOG_INFO("Received answer: " + answer->to_string(use_metta_as_query_tokens));
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
