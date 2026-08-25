#define LOG_LEVEL DEBUG_LEVEL

#include "AtomDBSingleton.h"
#include "JsonConfigParser.h"
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

static string start_test_case(const string& test_case) {
    LOG_INFO("--------------------------------------------------------------------------------");
    LOG_INFO("START " + test_case);
    return test_case;
}

static void finish_test_case(const string& test_case, bool success) {
    if (success) {
        LOG_INFO("OK - " + test_case);
    } else {
        LOG_INFO("FAILED - " + test_case);
    }
}

static bool assert_equal(unsigned int v1, unsigned int v2, const string& tag) {
    if (v1 == v2) {
        LOG_DEBUG("ASSERT PASSED - " + tag + " - " + to_string(v1) + " == " + to_string(v2));
        return true;
    } else {
        LOG_ERROR("ASSERT FAILED - " + tag + " - " + to_string(v1) + " != " + to_string(v2));
        return false;
    }
}

shared_ptr<LinkCreationProxy> make_proxy(BaseProxy::ORCHESTRATION_SCHEMA_TYPE orchestration = BaseProxy::NONE) {
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
    auto proxy = make_shared<LinkCreationProxy>(query_tokens, "", LinkCreatorRegistry::AND_TWO_PREDICATES, orchestration);
    proxy->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::MAX_UNPRODUCTIVE_VISITS_PER_ROUND] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::MAX_VISIT_ATTEMPTS_PER_ROUND] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::MAX_ROUNDS] = (unsigned int) 1;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_STRENGTH_THRESHOLD] = (double) 0;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] = (string) "";
    proxy->parameters[LinkCreationProxy::LOG_NEW_LINKS] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = (bool) true;
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = (bool) true;
    return proxy;
}

static bool test_and_two_predicates() {
    STACK_TRACE();

    string test_case = start_test_case("test_and_two_predicates()");
    bool success = true;

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

    auto proxy = make_proxy();

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    while (true) {
        if (proxy->finished()) {
            break;
        }
        shared_ptr<QueryAnswer> answer = proxy->pop();
        if (answer == nullptr) {
            Utils::sleep();
        }
    }
    unsigned int expected = 13207;
    success &= assert_equal(proxy->get_count(), expected, "link creation count");

    finish_test_case(test_case, success);
    return success;
}

static bool test_cycles() {
    STACK_TRACE();

    string test_case = start_test_case("test_cycles()");
    bool success = true;

    //unsigned int PROXIES = 5;
    //unsigned int CYCLES = 10;
    unsigned int PROXIES = 1;
    unsigned int CYCLES = 1;

    //unsigned int creations_per_cycle[PROXIES] = {100, 200, 300, 400, 500};
    //unsigned int num_cycles[PROXIES] = {5, 4, 3, 2, 1};
    //unsigned int total_creation[PROXIES] = {0, 0, 0, 0, 0};
    //unsigned int creation[PROXIES] = {0, 0, 0, 0, 0};
    //shared_ptr<LinkCreationProxy> proxy[PROXIES] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    unsigned int creations_per_cycle[PROXIES] = {100};
    unsigned int num_cycles[PROXIES] = {5};
    unsigned int total_creation[PROXIES] = {0};
    unsigned int creation[PROXIES] = {0};
    shared_ptr<LinkCreationProxy> proxy[PROXIES] = {nullptr, nullptr, nullptr, nullptr, nullptr};

    for (unsigned int i = 0; i < PROXIES; i++) {
        proxy[i] = make_proxy(BaseProxy::SYNC_ON_CYCLE_START);
        proxy[i]->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned int) 100; // XXXXX creations_per_cycle[i];
        proxy[i]->parameters[LinkCreationProxy::MAX_ROUNDS] = (unsigned int) num_cycles[i];
        proxy[i]->parameters[BaseProxy::ORCHESTRATION_SCHEMA] = (unsigned int) BaseProxy::SYNC_ON_CYCLE_START;
        ServiceBusSingleton::get_instance()->issue_bus_command(proxy[i]);
    }
    Utils::sleep(5000);
    for (unsigned int i = 0; i < PROXIES; i++) {
        proxy[i]->allow_cycle_start();
    }

    for (unsigned int j = 0; j < CYCLES; j++) {
        for (unsigned int i = 0; i < PROXIES; i++) {
            while (true) {
                if (proxy[i]->finished() || proxy[i]->get_waiting_flag()) {
                    break;
                }
                shared_ptr<QueryAnswer> answer = proxy[i]->pop();
                if (answer == nullptr) {
                    Utils::sleep();
                } else {
                    creation[i]++;
                    total_creation[i]++;
                }
            }
            success &= assert_equal(creation[i], creations_per_cycle[i], "creations in proxy[" + to_string(i) + "] in cycle " + to_string(j));
        }
    }

    finish_test_case(test_case, success);
    return success;
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
    bool success = true;
    //success &= test_and_two_predicates();
    success &= test_cycles();
    LOG_INFO("================================================================================");
    if (success) {
        LOG_INFO("OK - ALL TEST CASES PASSED");
        return 0;
    } else {
        LOG_INFO("FAILED - AT LEAST ONE TEST CASE FAILED");
        return 1;
    }
}
