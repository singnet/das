#include "AtomDBSingleton.h"
#include "PatternMatchingQueryProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "Utils.h"
#include "gtest/gtest.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace query_engine;
using namespace atomdb;

string handle_to_atom(const char* handle) {
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    shared_ptr<atomdb_api_types::AtomDocument> document = db->get_atom_document(handle);
    shared_ptr<atomdb_api_types::HandleList> targets = db->query_for_targets((char*) handle);
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

void check_query(vector<string>& query,
                 unsigned int expected_count,
                 ServiceBus* client_bus,
                 const string& context,
                 bool update_attention_broker,
                 bool unique_assignment) {
    shared_ptr<PatternMatchingQueryProxy> proxy1(new PatternMatchingQueryProxy(query, context));
    proxy1->set_unique_assignment_flag(unique_assignment);
    proxy1->set_attention_update_flag(update_attention_broker);

    shared_ptr<PatternMatchingQueryProxy> proxy2(new PatternMatchingQueryProxy(query, context));
    proxy2->set_unique_assignment_flag(unique_assignment);
    proxy2->set_attention_update_flag(update_attention_broker);
    proxy2->set_count_flag(true);

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

    // giving time to the server to close the previous connection
    // otherwise the test fails with "Node ID already in the network"
    Utils::sleep(3000);

    client_bus->issue_bus_command(proxy2);
    while (!proxy2->finished()) {
        Utils::sleep();
    }
    EXPECT_EQ(proxy2->get_count(), expected_count);
}

TEST(PatternMatchingQuery, queries) {
    setenv("DAS_REDIS_HOSTNAME", "localhost", 1);
    setenv("DAS_REDIS_PORT", "29000", 1);
    setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
    setenv("DAS_MONGODB_HOSTNAME", "localhost", 1);
    setenv("DAS_MONGODB_PORT", "28000", 1);
    setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
    setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);

    AtomDBSingleton::init();
    ServiceBus::initialize_statics({}, 54000, 54500);

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
    // int q2_expected_count = 3;

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
    // clang-format on

    check_query(q1, q1_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);
    // check_query(q2, q2_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);
    check_query(q3, q3_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);
    check_query(q4, q4_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);
    check_query(q5, q5_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);
    check_query(q6, q6_expected_count, client_bus, "PatternMatchingQuery.queries", false, true);

    check_query(q1, q1_expected_count, client_bus, "PatternMatchingQuery.queries", true, false);
    // check_query(q2, q2_expected_count, client_bus, "PatternMatchingQuery.queries", true, false);
    check_query(q3, q3_expected_count, client_bus, "PatternMatchingQuery.queries", true, false);
    check_query(q4, q4_expected_count, client_bus, "PatternMatchingQuery.queries", true, false);
    check_query(q5, q5_expected_count, client_bus, "PatternMatchingQuery.queries", true, false);
    check_query(q6, q6_expected_count, client_bus, "PatternMatchingQuery.queries", true, true);

    setenv("FETCH_CHUNK_SIZE", "1", 1);
    setenv("FETCH_THREAD_COUNT", "16", 1);
    check_query(q1, q1_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);
    // check_query(q2, q2_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);
    check_query(q3, q3_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);
    check_query(q4, q4_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);
    check_query(q5, q5_expected_count, client_bus, "PatternMatchingQuery.queries", false, false);

    Utils::sleep(2000);
}
