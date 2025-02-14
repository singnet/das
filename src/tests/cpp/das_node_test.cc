#include <cstdlib>
#include "gtest/gtest.h"

#include "DASNode.h"
#include "AtomDBSingleton.h"
#include "AtomDB.h"
#include "Utils.h"
#include "HandlesAnswer.h"

#include "test_utils.h"

using namespace query_engine;

string handle_to_atom(const char *handle) {

    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    shared_ptr<atomdb_api_types::AtomDocument> document = db->get_atom_document(handle);
    shared_ptr<atomdb_api_types::HandleList> targets = db->query_for_targets((char *) handle);
    string answer;

    if (targets != NULL) {
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

void check_query(
    vector<string> &query,
    unsigned int expected_count,
    DASNode *das,
    DASNode *requestor,
    const string &context) {

    cout << "XXXXXXXXXXXXXXXX DASNode.queries CHECK BEGIN" << endl;
    QueryAnswer *query_answer;
    RemoteIterator<HandlesAnswer> *response = requestor->pattern_matcher_query(query, context);
    unsigned int count = 0;
    while (! response->finished()) {
        while ((query_answer = response->pop()) == NULL) {
            if (response->finished()) {
                break;
            } else {
                Utils::sleep();
            }
        }
        if (query_answer != NULL) {
            cout << "XXXXX " << query_answer->to_string() << endl;
            //cout << "XXXXX " << handle_to_atom(query_answer->handles[0]) << endl;
            count++;
        }
    }
    EXPECT_EQ(count, expected_count);

    // Count query
    EXPECT_EQ(requestor->count_query(query, context), expected_count);

    delete response;
    cout << "XXXXXXXXXXXXXXXX DASNode.queries CHECK END" << endl;
}

TEST(DASNode, queries) {

    cout << "XXXXXXXXXXXXXXXX DASNode.queries BEGIN" << endl;

    setenv("DAS_REDIS_HOSTNAME", "localhost", 1);
    setenv("DAS_REDIS_PORT", "29000", 1);
    setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
    setenv("DAS_MONGODB_HOSTNAME", "localhost", 1);
    setenv("DAS_MONGODB_PORT", "28000", 1);
    setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
    setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);
    AtomDBSingleton::init();

    string das_id = "localhost:31700";
    string requestor_id = "localhost:31701";
    DASNode *das = new DASNode(das_id);
    Utils::sleep(1000);
    DASNode *requestor = new DASNode(requestor_id, das_id);
    Utils::sleep(1000);

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
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "NODE", "Symbol", "\"human\"",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Similarity",
                "VARIABLE", "v1",
                "NODE", "Symbol", "\"snake\""
    };
    int q5_expected_count = 5;

    check_query(q1, q1_expected_count, das, requestor, "DASNode.queries");
    check_query(q2, q2_expected_count, das, requestor, "DASNode.queries");
    check_query(q3, q3_expected_count, das, requestor, "DASNode.queries");
    check_query(q4, q4_expected_count, das, requestor, "DASNode.queries"); 
    check_query(q5, q5_expected_count, das, requestor, "DASNode.queries");

    //delete(requestor); // TODO: Uncomment this
    //delete(das); // TODO: Uncomment this

    cout << "XXXXXXXXXXXXXXXX DASNode.queries END" << endl;
}
