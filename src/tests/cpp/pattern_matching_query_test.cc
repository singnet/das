#include "Utils.h"
#include "ServiceBus.h"
#include "PatternMatchingQueryProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "AtomDBSingleton.h"
#include "gtest/gtest.h"

using namespace query_engine;
using namespace atomdb;


void check_query(vector<string>& query,
                 unsigned int expected_count,
                 ServiceBus* client_bus,
                 const string& context,
                 bool update_attention_broker) {

    cout << "XXXXXXXXXXXXXXXX DASNode.queries CHECK BEGIN" << endl;

    shared_ptr<PatternMatchingQueryProxy> proxy1(new PatternMatchingQueryProxy(
        query,
        context,
        update_attention_broker));

    shared_ptr<PatternMatchingQueryProxy> proxy2(new PatternMatchingQueryProxy(
        query,
        context,
        update_attention_broker,
        true));

    client_bus->issue_bus_command(proxy1);
    unsigned int count = 0;
    while (! proxy1->finished()) {
        if (proxy1->pop() != NULL) {
            count++;
        }
        Utils::sleep();
    }

    unique_ptr<HandlesAnswer> query_answer;
    while (!proxy1->finished()) {
        while (! (query_answer = proxy1->pop())) {
            if (proxy1->finished()) {
                break;
            } else {
                Utils::sleep();
            }
        }
        if (query_answer) {
            cout << "XXXXX " << query_answer->to_string() << endl;
            // cout << "XXXXX " << handle_to_atom(query_answer->handles[0]) << endl;
            count++;
        }
    }
    EXPECT_EQ(count, expected_count);
    EXPECT_EQ(proxy1->get_count(), expected_count);

    client_bus->issue_bus_command(proxy2);
    while (! proxy2->finished()) {
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


    ServiceBus::initialize_statics();
    string peer1_id = "localhost:33701";
    string peer2_id = "localhost:33702";

    ServiceBus server_bus(peer1_id);
    Utils::sleep(1000);
    ServiceBus client_bus(peer2_id, peer1_id);
    Utils::sleep(1000);
    server_bus.register_processor(make_shared<PatternMatchingQueryProcessor>());
    Utils::sleep(1000);

    vector<string> q1 = {"LINK_TEMPLATE",
                         "Expression",
                         "3",
                         "NODE",
                         "Symbol",
                         "Similarity",
                         "VARIABLE",
                         "v1",
                         "VARIABLE",
                         "v2"};
    int q1_expected_count = 14;

    vector<string> q2 = {"LINK_TEMPLATE",
                         "Expression",
                         "3",
                         "NODE",
                         "Symbol",
                         "Similarity",
                         "NODE",
                         "Symbol",
                         "\"human\"",
                         "VARIABLE",
                         "v1"};
    int q2_expected_count = 3;

    vector<string> q3 = {"AND",  "2",      "LINK_TEMPLATE", "Expression",    "3",
                         "NODE", "Symbol", "Similarity",    "VARIABLE",      "v1",
                         "NODE", "Symbol", "\"human\"",     "LINK_TEMPLATE", "Expression",
                         "3",    "NODE",   "Symbol",        "Inheritance",   "VARIABLE",
                         "v1",   "NODE",   "Symbol",        "\"plant\""};
    int q3_expected_count = 1;

    vector<string> q4 = {"AND",      "2",      "LINK_TEMPLATE", "Expression", "3",
                         "NODE",     "Symbol", "Similarity",    "VARIABLE",   "v1",
                         "VARIABLE", "v2",     "LINK_TEMPLATE", "Expression", "3",
                         "NODE",     "Symbol", "Similarity",    "VARIABLE",   "v2",
                         "VARIABLE", "v3"};
    int q4_expected_count = 26;  // TODO: FIX THIS count should be == 1

    vector<string> q5 = {"OR",   "2",      "LINK_TEMPLATE", "Expression",    "3",
                         "NODE", "Symbol", "Similarity",    "VARIABLE",      "v1",
                         "NODE", "Symbol", "\"human\"",     "LINK_TEMPLATE", "Expression",
                         "3",    "NODE",   "Symbol",        "Similarity",    "VARIABLE",
                         "v1",   "NODE",   "Symbol",        "\"snake\""};
    int q5_expected_count = 5;

    check_query(q1, q1_expected_count, &client_bus, "DASNode.queries", false);
    check_query(q2, q2_expected_count, &client_bus, "DASNode.queries", false);
    check_query(q3, q3_expected_count, &client_bus, "DASNode.queries", false);
    check_query(q4, q4_expected_count, &client_bus, "DASNode.queries", false);
    check_query(q5, q5_expected_count, &client_bus, "DASNode.queries", false);

    check_query(q1, q1_expected_count, &client_bus, "DASNode.queries", true);
    check_query(q2, q2_expected_count, &client_bus, "DASNode.queries", true);
    check_query(q3, q3_expected_count, &client_bus, "DASNode.queries", true);
    check_query(q4, q4_expected_count, &client_bus, "DASNode.queries", true);
    check_query(q5, q5_expected_count, &client_bus, "DASNode.queries", true);
}
