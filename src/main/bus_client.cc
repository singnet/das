#include <signal.h>

#include "BaseQueryProxy.h"
#include "BusCommandRouterProxy.h"
#include "FitnessFunctionRegistry.h"
#include "Helper.h"
#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "Logger.h"
#include "Properties.h"
#include "ProxyFactory.h"
#include "RemoteAtomDB.h"
#include "ServiceBusSingleton.h"
#include "SystemParametersSingleton.h"
#include "Utils.h"

using namespace commons;
using namespace mains;
using namespace std;
using namespace command_router;
using namespace agents;

int main(int argc, char* argv[]) {
    try {
        auto required_cmd_args = {Helper::ENDPOINT, Helper::BUS_ENDPOINT, Helper::PORTS_RANGE};
        auto cmd_args = Utils::parse_command_line(argc, argv);
        if (cmd_args.find("help") != cmd_args.end()) {
            cout << Helper::help(Helper::processor_type_from_string(cmd_args[Helper::CLIENT]),
                                 ServiceCallerType::CLIENT)
                 << endl;
            return 0;
        }

        if (cmd_args.find(Helper::CLIENT) == cmd_args.end()) {
            if (cmd_args.find(Helper::SERVICE) != cmd_args.end()) {
                cmd_args[Helper::CLIENT] = cmd_args[Helper::SERVICE];
            } else {
                RAISE_ERROR("Required argument missing: " + Helper::CLIENT + " (or --service=)");
            }
        }

        if (cmd_args.find(Helper::CONFIG) == cmd_args.end()) {
            RAISE_ERROR("Required argument missing: " + Helper::CONFIG);
        }

        JsonConfig json_config = JsonConfigParser::load(cmd_args[Helper::CONFIG]);
        SystemParametersSingleton::init(json_config);

        if (cmd_args.find(Helper::ENDPOINT) == cmd_args.end()) {
            cmd_args[Helper::ENDPOINT] = "localhost:40000";
        }
        if (cmd_args.find(Helper::PORTS_RANGE) == cmd_args.end()) {
            cmd_args[Helper::PORTS_RANGE] = "52000:52999";
        }

        auto it_known_peer = Helper::arg_to_json_config_key.find(cmd_args[Helper::CLIENT]);
        if (it_known_peer != Helper::arg_to_json_config_key.end()) {
            if (cmd_args.find(Helper::BUS_ENDPOINT) == cmd_args.end()) {
                cmd_args[Helper::BUS_ENDPOINT] =
                    json_config.at_path(it_known_peer->second + ".endpoint").get<string>();
            }
        } else {
            RAISE_ERROR("Unknown client: " + cmd_args[Helper::CLIENT]);
        }

        if (cmd_args.find(Helper::ATTENTION_BROKER_ENDPOINT) == cmd_args.end()) {
            auto attention = json_config.at_path("agents.attention.endpoint");
            if (!attention.is_null()) {
                cmd_args[Helper::ATTENTION_BROKER_ENDPOINT] = attention.get<string>();
            }
        }

        Helper::merge_params_from_config(cmd_args, json_config);

        for (auto req_arg : required_cmd_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                RAISE_ERROR("Required argument missing: " + string(req_arg));
            }
        }

        auto required_args =
            Helper::get_required_arguments(cmd_args[Helper::CLIENT], ServiceCallerType::CLIENT);
        for (auto req_arg : required_args) {
            if (cmd_args.find(req_arg) == cmd_args.end()) {
                RAISE_ERROR("Required argument missing: " + string(req_arg));
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

        const bool is_router_client = cmd_args[Helper::CLIENT] == "bus-command-router" ||
                                      Helper::processor_type_from_string(cmd_args[Helper::CLIENT]) ==
                                          ProcessorType::BUS_COMMAND_ROUTER;

        shared_ptr<BaseProxy> proxy;
        if (is_router_client) {
            string router_cmd = props.get<string>(Helper::CMD);
            string router_arg = props.get<string>(Helper::ARG);
            router_arg = ProxyFactory::parse_metta_expression(router_arg);
            proxy = make_shared<BusCommandRouterProxy>(router_cmd, router_arg);
        } else {
            proxy = ProxyFactory::create_proxy(cmd_args[Helper::CLIENT], props);
        }
        if (proxy == nullptr) {
            RAISE_ERROR("Could not create proxy for service or client is inactive: " +
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

        if (is_router_client) {
            auto router_proxy = dynamic_pointer_cast<BusCommandRouterProxy>(proxy);
            string router_cmd = props.get<string>(Helper::CMD);
            bool use_metta_as_query_tokens =
                router_proxy->parameters.get_or<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS, true);

            auto router_finished_or_error = [&]() {
                return router_proxy->finished() || router_proxy->error_flag;
            };

            if (router_cmd == "get") {
                while (router_proxy->params_response.empty() && !router_finished_or_error() &&
                       Helper::is_running) {
                    Utils::sleep(100);
                }
                if (router_proxy->error_flag) {
                    return 1;
                }
                cout << router_proxy->params_response << endl;
            } else if (router_cmd == "set") {
                while (router_proxy->set_param_ack.empty() && !router_finished_or_error() &&
                       Helper::is_running) {
                    Utils::sleep(100);
                }
                if (router_proxy->error_flag) {
                    return 1;
                }
                cout << router_proxy->set_param_ack << endl;
            } else if (router_cmd == "query") {
                while (!router_proxy->routed_flag && !router_finished_or_error() && Helper::is_running) {
                    Utils::sleep(100);
                }
                if (router_proxy->error_flag) {
                    return 1;
                }
                LOG_INFO("Query routed; waiting for answers...");
                while (!router_proxy->finished() && Helper::is_running) {
                    shared_ptr<QueryAnswer> answer;
                    while ((answer = router_proxy->pop()) != nullptr) {
                        LOG_INFO("Received answer: " + answer->to_string(use_metta_as_query_tokens));
                        for (string handle : answer->get_handles_vector()) {
                            if (answer->metta_expression.find(handle) !=
                                answer->metta_expression.end()) {
                                LOG_INFO(answer->metta_expression[handle]);
                            }
                        }
                    }
                    Utils::sleep(100);
                }
                if (router_proxy->error_flag) {
                    return 1;
                }
            } else if (router_cmd == "evolution") {
                while (!router_proxy->routed_flag && !router_finished_or_error() && Helper::is_running) {
                    Utils::sleep(100);
                }
                if (router_proxy->error_flag) {
                    return 1;
                }
                LOG_INFO("Evolution routed; waiting for results...");
                while (!router_proxy->finished() && Helper::is_running) {
                    shared_ptr<QueryAnswer> answer;
                    while ((answer = router_proxy->pop()) != nullptr) {
                        LOG_INFO("Received answer: " + answer->to_string(use_metta_as_query_tokens));
                        for (string handle : answer->get_handles_vector()) {
                            if (answer->metta_expression.find(handle) !=
                                answer->metta_expression.end()) {
                                LOG_INFO(answer->metta_expression[handle]);
                            }
                        }
                    }
                    Utils::sleep(100);
                }
                if (router_proxy->error_flag) {
                    return 1;
                }
            } else {
                RAISE_ERROR("Unknown router cmd: " + router_cmd);
            }
            return 0;
        }

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

        bool use_metta_as_query_tokens =
            cmd_args.find(Helper::USE_METTA_AS_QUERY_TOKENS) != cmd_args.end() &&
            cmd_args[Helper::USE_METTA_AS_QUERY_TOKENS] == "true";

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
        return 1;
    }
    return 0;
}
