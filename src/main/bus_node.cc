#include <signal.h>

#include "AttentionBrokerClient.h"
#include "EnvironmentRestoreGuard.h"
#include "FitnessFunctionRegistry.h"
#include "Helper.h"
#include "JsonConfigParser.h"
#include "Logger.h"
#include "ProcessorFactory.h"
#include "Properties.h"
#include "RemoteAtomDB.h"
#include "Utils.h"

using namespace commons;
using namespace mains;
using namespace std;
using namespace attention_broker;

namespace {

map<string, string> atomdb_env_from_config(const JsonConfig& config) {
    map<string, string> env;
    auto set_if = [&](const string& env_var, const string& path) {
        auto val = config.at_path(path);
        if (!val.is_null()) env[env_var] = (*val).is_string() ? val.get<string>() : (*val).dump();
    };
    set_if("DAS_REDIS_HOSTNAME", "atomdb.redis.hostname");
    set_if("DAS_REDIS_PORT", "atomdb.redis.port");
    set_if("DAS_USE_REDIS_CLUSTER", "atomdb.redis.cluster");
    set_if("DAS_MONGODB_HOSTNAME", "atomdb.mongodb.hostname");
    set_if("DAS_MONGODB_PORT", "atomdb.mongodb.port");
    set_if("DAS_MONGODB_USERNAME", "atomdb.mongodb.username");
    set_if("DAS_MONGODB_PASSWORD", "atomdb.mongodb.password");
    set_if("DAS_MORK_HOSTNAME", "atomdb.morkdb.hostname");
    set_if("DAS_MORK_PORT", "atomdb.morkdb.port");
    return env;
}

}  // namespace

void ctrl_c_handler(int) {
    LOG_INFO("Stopping service...");
    LOG_INFO("Done.");
    exit(0);
}

int main(int argc, char* argv[]) {
    try {
        auto required_cmd_args = {Helper::SERVICE};
        auto cmd_args = Utils::parse_command_line(argc, argv);
        auto it_config = cmd_args.find("config");

        ///////// Loading JSON config
        JsonConfig json_config;
        if (it_config != cmd_args.end() && !it_config->second.empty()) {
            json_config = JsonConfigParser::load(it_config->second);
        } else {
            json_config = JsonConfigParser::load(JsonConfig::JSON_CONFIG_DEFAULT_PATH, false);
        }
        // Map service name (e.g. "query-engine") to config section path (e.g. "agents.query")
        string service_name = cmd_args[Helper::SERVICE];
        auto it_section = Helper::arg_to_json_config_key.find(service_name);
        if (it_section != Helper::arg_to_json_config_key.end()) {
            const string& section_path = it_section->second;
            if (cmd_args.find(Helper::ENDPOINT) == cmd_args.end()) {
                auto endpoint_val = json_config.at_path(section_path + ".endpoint");
                cmd_args[Helper::ENDPOINT] = endpoint_val.get<string>();
            }
            if (cmd_args.find(Helper::PORTS_RANGE) == cmd_args.end()) {
                auto ports_val = json_config.at_path(section_path + ".ports_range");
                cmd_args[Helper::PORTS_RANGE] = ports_val.get<string>();
            }
        }

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
                auto it_path = Helper::arg_to_json_config_key.find(req_arg);
                if (it_path != Helper::arg_to_json_config_key.end()) {
                    auto val = json_config.at_path(it_path->second);
                    cmd_args[req_arg] = val.get<string>();
                } else {
                    Utils::error("Required argument missing: " + string(req_arg));
                }
            }
        }
        ///////// Handling signals
        signal(SIGINT, ctrl_c_handler);
        signal(SIGTERM, ctrl_c_handler);
        ///////// Initializing dependencies
        LOG_INFO("Starting service: " + cmd_args[Helper::SERVICE]);
        auto props = Properties(cmd_args.begin(), cmd_args.end());
        if (props.find(Helper::ATTENTION_BROKER_ENDPOINT) != props.end()) {
            AttentionBrokerClient::set_server_address(
                props.get<string>(Helper::ATTENTION_BROKER_ENDPOINT));
        }

        ///////// Initializing AtomDB
        EnvironmentRestoreGuard env_guard;
        env_guard.save_and_apply(atomdb_env_from_config(json_config));

        shared_ptr<AtomDB> atomdb = nullptr;
        if (props.get_or<string>(Helper::USE_MORK, "false") == "true") {
            atomdb = make_shared<MorkDB>();
        } else {
            string atomdb_type = json_config.at_path("atomdb.type").get_or<string>("");
            if (atomdb_type == "morkdb") {
                atomdb = make_shared<MorkDB>();
            } else if (atomdb_type == "remotedb") {
                auto remote_peers_val = json_config.at_path("atomdb.remote_peers");
                JsonConfig peers_config =
                    remote_peers_val.is_null() ? nlohmann::json() : *remote_peers_val;
                atomdb = make_shared<RemoteAtomDB>(peers_config);
            } else if (atomdb_type == "redismongodb" || atomdb_type.empty()) {
                atomdb = make_shared<RedisMongoDB>();
            }
        }
        AtomDBSingleton::provide(atomdb);
        env_guard.restore();

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
