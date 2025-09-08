#include "Context.h"

#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "Hasher.h"
#include "TestConfig.h"
#include "UntypedVariable.h"
#include "Utils.h"
#include "gtest/gtest.h"
#include "test_utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace atom_space;
using namespace attention_broker;

class TestContext : public Context {
   public:
    TestContext(const string& name, Atom& atom_key) : Context(name, atom_key) {}
};

TEST(Context, basics) {
    TestConfig::load_environment();
    AtomDBSingleton::init();

    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    random_init();

    string node1 = Hasher::node_handle("Symbol", "\"human\"");               // 0
    string node2 = Hasher::node_handle("Symbol", "\"monkey\"");              // 1
    string node3 = Hasher::node_handle("Symbol", "\"chimp\"");               // 2
    string node4 = Hasher::node_handle("Symbol", "\"ent\"");                 // 3
    string node5 = Hasher::node_handle("Symbol", "Similarity");              // 4
    string link1 = Hasher::link_handle("Expression", {node5, node1, node2}); // 5
    string link2 = Hasher::link_handle("Expression", {node5, node1, node3}); // 6
    string link3 = Hasher::link_handle("Expression", {node5, node1, node4}); // 7

    vector<string> tokens = {"LINK_TEMPLATE",
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
    vector<string> handles = {node1, node2, node3, node4, node5, link1, link2, link3};
    vector<float> importance;

    // Toplevel Atom based context
    LinkSchema atom_key(tokens);
    TestContext* context1 = new TestContext(random_handle(), atom_key);

    importance.clear();
    AttentionBrokerClient::get_importance(handles, context1->get_key(), importance);
    for (unsigned int i = 0; i < handles.size(); i++) {
        if (i <= 4) {
            EXPECT_TRUE(importance[i] == 0);
        } else {
            EXPECT_TRUE(importance[i] > 0);
        }
        cout << "importance[" << i << "]: " << importance[i] << endl;
    }
    EXPECT_TRUE(double_equals(importance[5], importance[6]));
    EXPECT_TRUE(double_equals(importance[5], importance[7]));

    AttentionBrokerClient::stimulate({{node2, 1}}, context1->get_key());
    importance.clear();
    AttentionBrokerClient::get_importance(handles, context1->get_key(), importance);
    for (unsigned int i = 0; i < handles.size(); i++) {
        cout << "importance[" << i << "]: " << importance[i] << endl;
    }
    EXPECT_TRUE(importance[0] == 0);
    EXPECT_TRUE(importance[1] > 0);
    EXPECT_TRUE(importance[2] == 0);
    EXPECT_TRUE(importance[3] == 0);
    EXPECT_TRUE(importance[4] == 0);
    EXPECT_TRUE(importance[5] > 0);
    EXPECT_TRUE(importance[6] > 0);
    EXPECT_TRUE(importance[7] > 0);
    EXPECT_TRUE(importance[5] > importance[6]);
    EXPECT_TRUE(double_equals(importance[6], importance[7]));
}
