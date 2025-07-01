#include <gtest/gtest.h>

#include "Properties.h"
#include "atom_space/AtomSpaceTypes.h"
#include "expression_hasher.h"

using namespace std;
using namespace atomspace;
using namespace commons;

class TestDecoder : public HandleDecoder {
   public:
    map<string, shared_ptr<Atom>> atoms;
    shared_ptr<Atom> get_atom(const string& handle) { return this->atoms[handle]; }
    shared_ptr<Atom> add_atom(shared_ptr<Atom> atom) {
        this->atoms[atom->handle()] = atom;
        return atom;
    }
};

TEST(NodeTest, NodeValidationAndToString) {
    EXPECT_NO_THROW(Node("Type", "Name"));
    EXPECT_THROW(Node("", "Name"), runtime_error);
    EXPECT_THROW(Node("__UNDEFINED_TYPE__", "Name"), runtime_error);

    EXPECT_NO_THROW(Node("Type", "Name"));
    EXPECT_THROW(Node("Type", ""), runtime_error);
    EXPECT_THROW(Node("", "Name"), runtime_error);

    Node n("Type", "Name");
    string s = n.to_string();
    EXPECT_EQ(s, "Node(type: 'Type', name: 'Name', custom_attributes: {})");
}

TEST(NodeTest, NodeComputeHandle) {
    unique_ptr<char, HandleDeleter> handle(::terminal_hash((char*) "Type", (char*) "Name"));
    ASSERT_NE(handle.get(), nullptr);
}

TEST(LinkTest, LinkValidation) {
    TestDecoder db;
    auto n1 = db.add_atom(make_shared<Node>("Type1", "Name1"));
    auto n2 = db.add_atom(make_shared<Node>("Type2", "Name2"));
    vector<string> targets = {n1->handle(), n2->handle()};
    EXPECT_NO_THROW(Link("LinkType", targets));
    vector<string> empty_targets;
    EXPECT_THROW(Link("LinkType", empty_targets), runtime_error);
    EXPECT_THROW(Link("", targets), runtime_error);
    EXPECT_THROW(Link("__UNDEFINED_TYPE__", targets), runtime_error);
}

TEST(LinkTest, LinkToString) {
    TestDecoder db;
    auto n1 = db.add_atom(make_shared<Node>("Type1", "Name1"));
    auto n2 = db.add_atom(make_shared<Node>("Type2", "Name2"));
    vector<string> targets = {n1->handle(), n2->handle()};
    Link l("LinkType", targets);
    string s = l.to_string();
    // clang-format off
    EXPECT_EQ(
        s,
        "Link("
            "type: 'LinkType', "
            "targets: [" + n1->handle() + ", " + n2->handle() + "], "
            "custom_attributes: {}"
        ")"
    );
    // clang-format on
}

TEST(NodeTest, NodeWithCustomAttributes) {
    Properties attrs;
    attrs["foo"] = std::string("bar");
    attrs["num"] = 42L;
    Node n("Type", "Name", attrs);
    EXPECT_EQ(n.custom_attributes.get_ptr<std::string>("foo") != nullptr, true);
    EXPECT_EQ(n.custom_attributes.get_ptr<long>("num") != nullptr, true);
    std::string s = n.to_string();
    EXPECT_EQ(s, "Node(type: 'Type', name: 'Name', custom_attributes: {foo: 'bar', num: 42})");
}

TEST(LinkTest, LinkWithCustomAttributes) {
    TestDecoder db;
    auto n1 = db.add_atom(make_shared<Node>("Type1", "Name1"));
    auto n2 = db.add_atom(make_shared<Node>("Type2", "Name2"));
    std::vector<string> targets = {n1->handle(), n2->handle()};
    Link l("LinkType", targets, {{"flag", true}, {"count", 10}});
    EXPECT_EQ(l.custom_attributes.get_ptr<bool>("flag") != nullptr, true);
    std::string s = l.to_string();
    // clang-format off
    EXPECT_EQ(
        s,
        "Link("
            "type: 'LinkType', "
            "targets: [" + n1->handle() + ", " + n2->handle() + "], "
            "custom_attributes: {count: 10, flag: true}"
        ")"
    );
    // clang-format on
}

TEST(WildcardTest, Wildcards) {
    TestDecoder db;
    UntypedVariable v1("v1");
    UntypedVariable v2("v2");
    UntypedVariable v3 = v1;
    UntypedVariable v4(v2);

    EXPECT_TRUE(v1 == v3);
    EXPECT_TRUE(v1 != v2);
    EXPECT_TRUE(v2 == v4);
    EXPECT_TRUE(v4 != v3);
    EXPECT_EQ(v1.metta_representation(db), string("$v1"));
    EXPECT_EQ(v4.metta_representation(db), string("$v2"));
    EXPECT_TRUE(v1.handle() == v3.handle());
    EXPECT_TRUE(v1.handle() != v2.handle());
    EXPECT_TRUE(v2.handle() == v4.handle());
    EXPECT_TRUE(v4.handle() != v3.handle());
}

TEST(LinkTest, CompositeTypes) {
    TestDecoder db;
    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;
    vector<string> v;

    auto node1 = db.add_atom(make_shared<Node>(symbol, string("n1")));
    auto node2 = db.add_atom(make_shared<Node>(symbol, string("n2")));
    auto node3 = db.add_atom(make_shared<Node>(symbol, string("n3")));
    auto node4 = db.add_atom(make_shared<Node>(symbol, string("n4")));

    v = {node1->handle(), node2->handle()};
    auto link1 = db.add_atom(make_shared<Link>(expression, v));
    v = {node3->handle(), link1->handle()};
    auto link2 = db.add_atom(make_shared<Link>(expression, v));
    v = {link2->handle(), node4->handle()};
    auto link3 = db.add_atom(make_shared<Link>(expression, v));

    string symbol_hash = string(named_type_hash((char*) symbol.c_str()));
    string expression_hash = string(named_type_hash((char*) expression.c_str()));

    EXPECT_EQ(node1->composite_type_hash(db), symbol_hash);
    EXPECT_EQ(node2->composite_type_hash(db), symbol_hash);
    EXPECT_EQ(node3->composite_type_hash(db), symbol_hash);
    EXPECT_EQ(node4->composite_type_hash(db), symbol_hash);

    vector<string> link1_composite = link1->composite_type(db);
    string link1_composite_hash = link1->composite_type_hash(db);
    vector<string> link2_composite = link2->composite_type(db);
    string link2_composite_hash = link2->composite_type_hash(db);
    vector<string> link3_composite = link3->composite_type(db);
    string link3_composite_hash = link3->composite_type_hash(db);

    // clang-format off
    /*
    cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
    cout << "node1: " << node1->handle() << endl;
    cout << "node2: " << node2->handle() << endl;
    cout << "node3: " << node3->handle() << endl;
    cout << "node4: " << node4->handle() << endl;
    cout << "Expression: " << expression_hash << " Symbol: " << symbol_hash << endl;
    cout << "----------------------------------------------" << endl;
    cout << "link1: " << link1->handle() << " " << link1->to_string() << endl;
    cout << link1_composite.size() << " " << link1_composite[0] << " " << link1_composite[1] << " " << link1_composite[2] << endl;
    cout << link1_composite_hash << endl;
    cout << "----------------------------------------------" << endl;
    cout << "link2: " << link2->handle() << " " << link2->to_string() << endl;
    cout << link2_composite.size() << " " << link2_composite[0] << " " << link2_composite[1] << " " << link2_composite[2] << endl;
    cout << link2_composite_hash << endl;
    cout << "----------------------------------------------" << endl;
    cout << "link3: " << link3->handle() << " " << link3->to_string() << endl;
    cout << link3_composite.size() << " " << link3_composite[0] << " " << link3_composite[1] << " " << link3_composite[2] << endl;
    cout << link3_composite_hash << endl;
    cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
    */
    // clang-format on

    EXPECT_TRUE(link1_composite == vector<string>({expression_hash, symbol_hash, symbol_hash}));
    EXPECT_TRUE(link2_composite == vector<string>({expression_hash, symbol_hash, link1_composite_hash}));
    EXPECT_TRUE(link3_composite == vector<string>({expression_hash, link2_composite_hash, symbol_hash}));
}

TEST(LinkTest, MettaStrings) {

    TestDecoder db;
    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;
    vector<string> v;

    auto node1 = db.add_atom(make_shared<Node>(symbol, string("n1")));
    auto node2 = db.add_atom(make_shared<Node>(symbol, string("n2")));
    auto node3 = db.add_atom(make_shared<Node>(symbol, string("n3")));
    auto node4 = db.add_atom(make_shared<Node>(symbol, string("n4")));
    auto node5 = db.add_atom(make_shared<Node>("blah", string("n4")));

    v = {node1->handle(), node2->handle()};
    auto link1 = db.add_atom(make_shared<Link>(expression, v));
    v = {node3->handle(), link1->handle()};
    auto link2 = db.add_atom(make_shared<Link>(expression, v));
    v = {link2->handle(), node4->handle()};
    auto link3 = db.add_atom(make_shared<Link>(expression, v));
    v = {node1->handle(), node5->handle()};
    auto link4 = db.add_atom(make_shared<Link>(expression, v));
    v = {node1->handle(), node2->handle()};
    auto link5 = db.add_atom(make_shared<Link>("blah", v));

    EXPECT_EQ(node1->metta_representation(db), string("n1"));
    EXPECT_EQ(node2->metta_representation(db), string("n2"));
    EXPECT_EQ(node3->metta_representation(db), string("n3"));
    EXPECT_EQ(node4->metta_representation(db), string("n4"));
    EXPECT_EQ(link1->metta_representation(db), string("(n1 n2)"));
    EXPECT_EQ(link2->metta_representation(db), string("(n3 (n1 n2))"));
    EXPECT_EQ(link3->metta_representation(db), string("((n3 (n1 n2)) n4)"));
    EXPECT_THROW(node5->metta_representation(db), runtime_error);
    EXPECT_THROW(link4->metta_representation(db), runtime_error);
    EXPECT_THROW(link5->metta_representation(db), runtime_error);
}

TEST(LinkTest, CopyAndComparisson) {
    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;

    Node node1(symbol, "n1");
    Node node2(symbol, "n2");
    Node node3("blah", "n1");
    Node node4("blah", "n1");
    Node node5(node1);
    Node node6 = node4;

    Link link1(expression, {node1.handle(), node3.handle()});
    Link link2(expression, {node3.handle(), node1.handle()});
    Link link3("blah", {node1.handle(), node3.handle()});
    Link link4(expression, {node1.handle(), node4.handle()});
    Link link5(link2);
    Link link6 = link3;

    EXPECT_TRUE(node1 == node1);
    EXPECT_TRUE(node1 != node2);
    EXPECT_TRUE(node1 != node3);
    EXPECT_TRUE(node1 != node4);
    EXPECT_TRUE(node1 == node5);
    EXPECT_TRUE(node1 != node6);

    EXPECT_TRUE(node2 != node1);
    EXPECT_TRUE(node2 == node2);
    EXPECT_TRUE(node2 != node3);
    EXPECT_TRUE(node2 != node4);
    EXPECT_TRUE(node2 != node5);
    EXPECT_TRUE(node2 != node6);

    EXPECT_TRUE(node3 != node1);
    EXPECT_TRUE(node3 != node2);
    EXPECT_TRUE(node3 == node3);
    EXPECT_TRUE(node3 == node4);
    EXPECT_TRUE(node3 != node5);
    EXPECT_TRUE(node3 == node6);

    EXPECT_TRUE(node4 != node1);
    EXPECT_TRUE(node4 != node2);
    EXPECT_TRUE(node4 == node3);
    EXPECT_TRUE(node4 == node4);
    EXPECT_TRUE(node4 != node5);
    EXPECT_TRUE(node4 == node6);

    EXPECT_TRUE(node5 == node1);
    EXPECT_TRUE(node5 != node2);
    EXPECT_TRUE(node5 != node3);
    EXPECT_TRUE(node5 != node4);
    EXPECT_TRUE(node5 == node5);
    EXPECT_TRUE(node5 != node6);

    EXPECT_TRUE(node6 != node1);
    EXPECT_TRUE(node6 != node2);
    EXPECT_TRUE(node6 == node3);
    EXPECT_TRUE(node6 == node4);
    EXPECT_TRUE(node6 != node5);
    EXPECT_TRUE(node6 == node6);

    EXPECT_TRUE(link1 == link1);
    EXPECT_TRUE(link1 != link2);
    EXPECT_TRUE(link1 != link3);
    EXPECT_TRUE(link1 == link4);
    EXPECT_TRUE(link1 != link5);
    EXPECT_TRUE(link1 != link6);

    EXPECT_TRUE(link2 != link1);
    EXPECT_TRUE(link2 == link2);
    EXPECT_TRUE(link2 != link3);
    EXPECT_TRUE(link2 != link4);
    EXPECT_TRUE(link2 == link5);
    EXPECT_TRUE(link2 != link6);

    EXPECT_TRUE(link3 != link1);
    EXPECT_TRUE(link3 != link2);
    EXPECT_TRUE(link3 == link3);
    EXPECT_TRUE(link3 != link4);
    EXPECT_TRUE(link3 != link5);
    EXPECT_TRUE(link3 == link6);

    EXPECT_TRUE(link4 == link1);
    EXPECT_TRUE(link4 != link2);
    EXPECT_TRUE(link4 != link3);
    EXPECT_TRUE(link4 == link4);
    EXPECT_TRUE(link4 != link5);
    EXPECT_TRUE(link4 != link6);

    EXPECT_TRUE(link5 != link1);
    EXPECT_TRUE(link5 == link2);
    EXPECT_TRUE(link5 != link3);
    EXPECT_TRUE(link5 != link4);
    EXPECT_TRUE(link5 == link5);
    EXPECT_TRUE(link5 != link6);

    EXPECT_TRUE(link6 != link1);
    EXPECT_TRUE(link6 != link2);
    EXPECT_TRUE(link6 == link3);
    EXPECT_TRUE(link6 != link4);
    EXPECT_TRUE(link6 != link5);
    EXPECT_TRUE(link6 == link6);
}
