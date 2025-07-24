#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "PatternMatchingQueryProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "TestConfig.h"
#include "Utils.h"
#include "gtest/gtest.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace query_engine;
using namespace atomdb;

string handle_to_atom(const string& handle) {
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    shared_ptr<atomdb_api_types::AtomDocument> document = db->get_atom_document(handle);
    shared_ptr<atomdb_api_types::HandleList> targets = db->query_for_targets(handle);
    string answer;

    if (targets != nullptr) {
        // is link
        answer += "<";
        answer += document->get("named_type");
        answer += ": [";
        for (unsigned int i = 0; i < targets->size(); i++) {
            answer += handle_to_atom(targets->get_handle(i));
            if (i < (targets->size() - 1)) {
                answer += ", ";
            }
        }
        answer += ">";
    } else {
        // is node
        answer += "(";
        answer += document->get("named_type");
        answer += ": ";
        answer += document->get("name");
        answer += ")";
    }

    return answer;
}

void check_query(const string& query_tag,
                 const vector<string>& query,
                 unsigned int expected_count,
                 ServiceBus* client_bus,
                 const string& context,
                 bool update_attention_broker,
                 bool unique_assignment,
                 bool positive_importance,
                 bool error_flag) {
    shared_ptr<PatternMatchingQueryProxy> proxy1(new PatternMatchingQueryProxy(query, context));
    proxy1->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = unique_assignment;
    proxy1->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = update_attention_broker;
    proxy1->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    proxy1->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = positive_importance;
    LOG_INFO("==================== Query tag: " + query_tag);
    LOG_INFO("proxy1: " + proxy1->to_string());

    shared_ptr<PatternMatchingQueryProxy> proxy2(new PatternMatchingQueryProxy(query, context));
    proxy2->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = unique_assignment;
    proxy2->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = update_attention_broker;
    proxy2->parameters[PatternMatchingQueryProxy::COUNT_FLAG] = true;
    proxy2->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = positive_importance;
    LOG_INFO("proxy2: " + proxy2->to_string());

    client_bus->issue_bus_command(proxy1);
    unsigned int count = 0;
    shared_ptr<QueryAnswer> query_answer;
    while (!proxy1->finished()) {
        while (!(query_answer = proxy1->pop())) {
            if (proxy1->finished()) {
                break;
            } else {
                Utils::sleep();
            }
        }
        if (query_answer) {
            LOG_INFO(">>>>>>>>>> " << query_answer->assignment.to_string());
            count++;
        }
    }
    EXPECT_EQ(count, expected_count);
    EXPECT_EQ(proxy1->get_count(), expected_count);
    EXPECT_EQ(proxy1->error_flag, error_flag);

    // giving time to the server to close the previous connection
    // otherwise the test fails with "Node ID already in the network"
    Utils::sleep(3000);

    client_bus->issue_bus_command(proxy2);
    while (!proxy2->finished()) {
        Utils::sleep();
    }
    EXPECT_EQ(proxy2->get_count(), expected_count);
    EXPECT_EQ(proxy2->error_flag, error_flag);
}

TEST(PatternMatchingQuery, queries) {
    TestConfig::load_environment();

    AtomDBSingleton::init();
    ServiceBus::initialize_statics({}, 57000, 57500);

    string peer1_id = "localhost:33701";
    string peer2_id = "localhost:33702";

    ServiceBus* server_bus = new ServiceBus(peer1_id);
    Utils::sleep(500);
    server_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());
    Utils::sleep(500);
    ServiceBus* client_bus = new ServiceBus(peer2_id, peer1_id);
    Utils::sleep(500);

    // clang-format off
    vector<string> q1 = {
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Similarity",
            "VARIABLE", "v1",
            "VARIABLE", "v2"
    };
    int q1_expected_count = 14;

    vector<string> q2 = {
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Similarity",
            "NODE", "Symbol", "\"human\"",
            "VARIABLE", "v1"
    };
    int q2_expected_count = 3;

    vector<string> q3 = {
        "AND", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "NODE", "Symbol", "\"human\"",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Inheritance",
                "VARIABLE", "v1",
                "NODE", "Symbol", "\"plant\""
    };
    int q3_expected_count = 1;

    vector<string> q4 = {
        "AND", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "VARIABLE", "v2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v2",
                "VARIABLE", "v3"
    };
    int q4_expected_count = 26;  // TODO: FIX THIS count should be == 1

    vector<string> q5 = {
        "OR", "2",
            "LINK_TEMPLATE", "Expression",    "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "NODE", "Symbol", "\"human\"",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "NODE", "Symbol", "\"snake\""
    };
    int q5_expected_count = 5;
    
    vector<string> q6 = {
        "OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "NODE", "Symbol", "\"human\"",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "NODE", "Symbol", "\"chimp\""
    };
    int q6_expected_count = 4;
    
    vector<string> q7 = {
        "OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "NODE", "Symbol", "\"human\"",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "ATOM", Hasher::node_handle("Symbol", "\"chimp\"")
    };
    int q7_expected_count = 4;

    // Regular queries
    check_query("q1", q1, q1_expected_count, client_bus, "PatternMatchingQuery.queries", false, false, false, false);
    check_query("q2", q2, q2_expected_count, client_bus, "PatternMatchingQuery.queries", false, false, false, false);
    check_query("q3", q3, q3_expected_count, client_bus, "PatternMatchingQuery.queries", false, false, false, false);
    check_query("q4", q4, q4_expected_count, client_bus, "PatternMatchingQuery.queries", false, false, false, false);
    check_query("q5", q5, q5_expected_count, client_bus, "PatternMatchingQuery.queries", false, false, false, false);
    check_query("q6", q6, q6_expected_count, client_bus, "PatternMatchingQuery.queries", false, true, false, false);
    check_query("q7", q7, q7_expected_count, client_bus, "PatternMatchingQuery.queries", false, true, false, false);

    // Importance filtering
    check_query("filtered q2", q2, q2_expected_count, client_bus, "PatternMatchingQuery.queries", true, false, false, false);
    check_query("filtered q1", q1, 3, client_bus, "PatternMatchingQuery.queries", false, false, true, false);

    // Remote exception
    check_query("invalid", {"BLAH"}, 0, client_bus, "PatternMatchingQuery.queries", false, false, false, true);

    // Metta Expression
    shared_ptr<PatternMatchingQueryProxy> proxy(new PatternMatchingQueryProxy(q3, "PatternMatchingQuery.queries"));
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = true;
    client_bus->issue_bus_command(proxy);
    unsigned int count = 0;
    shared_ptr<QueryAnswer> answer;
    while (!proxy->finished()) {
        while (!(answer = proxy->pop())) {
            if (proxy->finished()) {
                break;
            } else {
                Utils::sleep();
            }
        }
        if (answer) {
            count++;
            EXPECT_EQ(answer->metta_expression["434d4303c556d94d2774ff70a5140b23"], "(Inheritance \"ent\" \"plant\")");
            EXPECT_EQ(answer->metta_expression["d610ca5aa289bf532010cb6fe79f754e"], "(Similarity \"ent\" \"human\")");
            EXPECT_EQ(answer->metta_expression[answer->assignment.get("v1")], "\"ent\"");
        }
    }
    EXPECT_EQ(count, 1);

    Utils::sleep(2000);
    // clang-format on
}
