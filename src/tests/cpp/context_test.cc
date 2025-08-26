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

/*
TEST(Context, basics) {
    TestConfig::load_environment();
    AtomDBSingleton::init();
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();

    // (outter (intermediate (inner a) b) c)
    string node1 = db->add_node(new Node("Symbol", "outter"));
    string node2 = db->add_node(new Node("Symbol", "intermediate"));
    string node3 = db->add_node(new Node("Symbol", "inner"));
    string node4 = db->add_node(new Node("Symbol", "a"));
    string node5 = db->add_node(new Node("Symbol", "b"));
    string node6 = db->add_node(new Node("Symbol", "c"));
    string link1 = db->add_link(new Link("Expression", {node3, node4}));
    string link2 = db->add_link(new Link("Expression", {node2, link1, node5}));
    string link3 = db->add_link(new Link("Expression", {node1, link2, node6}, true));

    cout << "XXX node1: " << node1 << endl;
    cout << "XXX node2: " << node2 << endl;
    cout << "XXX node3: " << node3 << endl;
    cout << "XXX node4: " << node4 << endl;
    cout << "XXX node5: " << node5 << endl;
    cout << "XXX node6: " << node6 << endl;
    cout << "XXX link1: " << link1 << endl;
    cout << "XXX link2: " << link2 << endl;
    cout << "XXX link3: " << link3 << endl;

    db->delete_link(link3, true);

    //db->delete_link(link3);
    //db->delete_link(link2);
    //db->delete_link(link1);
    //db->delete_link(node1);
    //db->delete_link(node2);
    //db->delete_link(node3);
    //db->delete_link(node4);
    //db->delete_link(node5);
    //db->delete_link(node6);
}
*/

TEST(Context, basics) {
    TestConfig::load_environment();
    AtomDBSingleton::init();
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    random_init();

    string node1 = Hasher::node_handle("Symbol", "\"human\"");
    string node2 = Hasher::node_handle("Symbol", "\"monkey\"");
    string node3 = Hasher::node_handle("Symbol", "\"chimp\"");
    string node4 = Hasher::node_handle("Symbol", "\"ent\"");
    string node5 = Hasher::node_handle("Symbol", "Similarity");
    string link1 = Hasher::link_handle("Expression", {node5, node1, node2});
    string link2 = Hasher::link_handle("Expression", {node5, node1, node3});
    string link3 = Hasher::link_handle("Expression", {node5, node1, node4});

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
    LinkSchema atom_key(tokens);
    TestContext* context = new TestContext(random_handle(), atom_key);

    vector<string> handles = {node1, node2, node3, node4, node5, link1, link2, link3};
    vector<float> importance;
    vector<float> expected;

    AttentionBrokerClient::get_importance(handles, context->get_key(), importance);
    for (unsigned int i = 0; i < handles.size(); i++) {
        if (i <= 4) {
            EXPECT_TRUE(importance[i] == 0);
        } else {
            EXPECT_TRUE(importance[i] > 0);
        }
    }
    EXPECT_TRUE(double_equals(importance[5], importance[6]));
    EXPECT_TRUE(double_equals(importance[5], importance[7]));

    AttentionBrokerClient::stimulate({{node2, 1}}, context->get_key());
    importance.clear();
    AttentionBrokerClient::get_importance(handles, context->get_key(), importance);
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
