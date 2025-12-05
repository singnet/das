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
                 const string& metta_expression,
                 unsigned int expected_count,
                 ServiceBus* client_bus,
                 const string& context,
                 bool update_attention_broker,
                 bool unique_assignment,
                 bool positive_importance,
                 bool error_flag,
                 bool unique_value_flag) {
    LOG_INFO("==================== Query tag: " + query_tag);

    shared_ptr<PatternMatchingQueryProxy> proxy1(new PatternMatchingQueryProxy(query, context));
    proxy1->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = unique_assignment;
    proxy1->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = update_attention_broker;
    proxy1->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    proxy1->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = positive_importance;
    proxy1->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = unique_value_flag;
    LOG_INFO("proxy1: " + proxy1->to_string());

    shared_ptr<PatternMatchingQueryProxy> proxy2(new PatternMatchingQueryProxy(query, context));
    proxy2->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = unique_assignment;
    proxy2->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = update_attention_broker;
    proxy2->parameters[PatternMatchingQueryProxy::COUNT_FLAG] = true;
    proxy2->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = positive_importance;
    proxy2->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = unique_value_flag;
    LOG_INFO("proxy2: " + proxy2->to_string());

    vector<string> metta_query = {metta_expression};
    shared_ptr<PatternMatchingQueryProxy> proxy3(new PatternMatchingQueryProxy(metta_query, context));
    proxy3->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = true;
    proxy3->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = unique_assignment;
    proxy3->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = update_attention_broker;
    proxy3->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;
    proxy3->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = positive_importance;
    proxy3->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = unique_value_flag;
    LOG_INFO("proxy3: " + proxy3->to_string());

    unsigned int count = 0;
    shared_ptr<QueryAnswer> query_answer;

    client_bus->issue_bus_command(proxy1);
    count = 0;
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
            for (auto pair : query_answer->assignment.table) {
                LOG_INFO(">>>>>>>>>>>>>> " << pair.first << " " << handle_to_atom(pair.second));
            }
            count++;
        }
    }
    EXPECT_EQ(count, expected_count);
    EXPECT_EQ(proxy1->get_count(), expected_count);
    EXPECT_EQ(proxy1->error_flag, error_flag);

    // giving time to the server to close the previous connection
    // otherwise the test fails with "Node ID already in the network"
    Utils::sleep();

    client_bus->issue_bus_command(proxy2);
    while (!proxy2->finished()) {
        Utils::sleep();
    }
    EXPECT_EQ(proxy2->get_count(), expected_count);
    EXPECT_EQ(proxy2->error_flag, error_flag);

    // giving time to the server to close the previous connection
    // otherwise the test fails with "Node ID already in the network"
    Utils::sleep();

    if (metta_expression != "") {
        client_bus->issue_bus_command(proxy3);
        count = 0;
        while (!proxy3->finished()) {
            while (!(query_answer = proxy3->pop())) {
                if (proxy3->finished()) {
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
        EXPECT_EQ(proxy3->get_count(), expected_count);
        EXPECT_EQ(proxy3->error_flag, error_flag);
    }
}

TEST(PatternMatchingQuery, queries) {
    TestConfig::load_environment();

    AtomDBSingleton::init();
    ServiceBus::initialize_statics({}, 40200, 40299);

    string peer1_id = "localhost:40041";
    string peer2_id = "localhost:40042";

    ServiceBus* server_bus = new ServiceBus(peer1_id);
    Utils::sleep();
    server_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());
    Utils::sleep();
    ServiceBus* client_bus = new ServiceBus(peer2_id, peer1_id);
    Utils::sleep();

    // clang-format off
    vector<string> q1 = {
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Similarity",
            "VARIABLE", "v1",
            "VARIABLE", "v2"
    };
    string q1m = "(Similarity $v1 $v2)";
    int q1_expected_count = 14;

    vector<string> q2 = {
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Similarity",
            "NODE", "Symbol", "\"human\"",
            "VARIABLE", "v1"
    };
    string q2m = "(Similarity \"human\" $v1)";
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
    string q3m = "(and (Similarity $v1 \"human\") (Inheritance $v1 \"plant\"))";
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
    string q4m = "(and (Similarity $v1 $v2) (Similarity $v2 $v3))";
    int q4_expected_count = 12;  // TODO: FIX THIS count should be == 3?
                                 // The point is that different permutations
                                 // are being returned, e.g. (human, monkey, chimp)
                                 // and (chimp, human, monkey). These repetitions
                                 // doesn't violate the current query parameters
                                 // we have to avoid repetitions (UNIQUE_ASSIGNMENT
                                 // and UNIQUE_VALUE) but we may want to avoid them,
                                 // I'm not sure. Perhaps a third optional parameter?

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
    string q5m = "(or (Similarity $v1 \"human\") (Similarity $v1 \"snake\"))";
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
    string q6m = "(or (Similarity $v1 \"human\") (Similarity $v1 \"chimp\"))";
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
    string q7m = "";
    int q7_expected_count = 4;

    // Regular queries
    check_query("q1", q1, q1m, q1_expected_count, client_bus, "PatternMatchingQuery.queries", false, false, false, false, false);
    check_query("q2", q2, q2m, q2_expected_count, client_bus, "PatternMatchingQuery.queries", false, false, false, false, false);
    check_query("q3", q3, q3m, q3_expected_count, client_bus, "PatternMatchingQuery.queries", false, false, false, false, false);
    check_query("q4", q4, q4m, q4_expected_count, client_bus, "PatternMatchingQuery.queries", false, true, false, false, true);
    check_query("q4", q4, q4m, 26, client_bus, "PatternMatchingQuery.queries", false, false, false, false, false);
    check_query("q5", q5, q5m, q5_expected_count, client_bus, "PatternMatchingQuery.queries", false, false, false, false, false);
    check_query("q6", q6, q6m, q6_expected_count, client_bus, "PatternMatchingQuery.queries", false, true, false, false, false);
    check_query("q7", q7, q7m, q7_expected_count, client_bus, "PatternMatchingQuery.queries", false, true, false, false, false);

    // Importance filtering
    // XXX AttentionBroker is being revised so its dynamics is a bit unpredictable right now
    // XXX so we're disabling the following test cases for while because they rely on these
    // XXX dynamics.
    //check_query("filtered q2", q2, q2m, q2_expected_count, client_bus, "PatternMatchingQuery.queries", true, false, false, false, false);
    //check_query("filtered q1", q1, q1m, 3, client_bus, "PatternMatchingQuery.queries", false, false, true, false, false);

    // Remote exception
    check_query("invalid", {"BLAH"}, "", 0, client_bus, "PatternMatchingQuery.queries", false, false, false, true, false);

    // Metta expression in QueryAnswer
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

    // clang-format on
}
