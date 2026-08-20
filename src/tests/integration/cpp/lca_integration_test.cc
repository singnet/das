#include "AtomDBSingleton.h"
#include "JsonConfigParser.h"
#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"
#include "ServiceBusSingleton.h"
#include "SystemParametersSingleton.h"
#include "Utils.h"
#include "LinkCreationProxy.h"
#include "LinkCreatorRegistry.h"
#include "AndTwoPredicates.h"
#include "PatternMatchingQueryProxy.h"
#include "tags.h"

using namespace std;
using namespace atomdb;
using namespace commons;
using namespace service_bus;
using namespace link_creators;
using namespace link_creation_agent;

static void insert_type_symbols() {
    STACK_TRACE();
    vector<string> to_insert = {EQUIVALENCE_TAG, IMPLICATION_TAG, LOGICAL_AND_TAG};
    Node* node;
    for (string node_name : to_insert) {
        node = new Node(SYMBOL, node_name);
        AtomDBSingleton::get_instance()->add_node(node);
        delete (node);
    }
}

static void test_and_two_predicates() {
    STACK_TRACE();

    vector<string> query_tokens = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE2,
                VARIABLE, CONCEPT,
    };

    auto proxy = make_shared<LinkCreationProxy>(query_tokens, "", LinkCreatorRegistry::AND_TWO_PREDICATES);
    proxy->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::MAX_UNPRODUCTIVE_VISITS_PER_ROUND] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::MAX_VISIT_ATTEMPTS_PER_ROUND] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::MAX_ROUNDS] = (unsigned int) 1;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_STRENGTH_THRESHOLD] = (double) 0;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] = (string) "/tmp/test_and_two_predicates.metta";

    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = (bool) true;
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = (bool) true;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    while (true) {
        if (proxy->finished()) {
            break;
        }
        Utils::sleep();
    }
}

int main(int argc, char* argv[]) {
    STACK_TRACE();

    string config_file = "/opt/das/config/das.json";
    auto json_config = JsonConfigParser::load(config_file);
    string client_endpoint = "localhost:35700";
    string server_endpoint = json_config.at_path("agents.query.endpoint").get<string>();
    pair<unsigned int, unsigned int> ports_range = {35701, 35799};
    
    auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
    SystemParametersSingleton::init(json_config);
    AtomDBSingleton::init(atomdb_config);
    LinkCreatorRegistry::initialize_statics();
    ServiceBusSingleton::init(client_endpoint, server_endpoint, ports_range.first, ports_range.second);

    insert_type_symbols();
    test_and_two_predicates();

    return 0;
}
