#include <gtest/gtest.h>

#include "Atom.h"
#include "HandleDecoder.h"
#include "Hasher.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Logger.h"
#include "MettaMapping.h"
#include "Node.h"
#include "UntypedVariable.h"

using namespace std;
using namespace commons;
using namespace atoms;

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
    Link l("LinkType", targets, true);
    string s = l.to_string();
    // clang-format off
    EXPECT_EQ(
        s,
        "Link("
            "type: 'LinkType', "
            "targets: [" + n1->handle() + ", " + n2->handle() + "], "
            "is_toplevel: true, "
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
    vector<string> tokens;
    n.tokenize(tokens);
    Node node_copy(tokens);
    EXPECT_TRUE(node_copy == n);
    s = node_copy.to_string();
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
            "is_toplevel: false, "
            "custom_attributes: {count: 10, flag: true}"
        ")"
    );
    // clang-format on
    vector<string> tokens;
    l.tokenize(tokens);
    Link link_copy(tokens);
    EXPECT_TRUE(link_copy == l);
    s = link_copy.to_string();
    EXPECT_EQ(
        s,
        "Link("
            "type: 'LinkType', "
            "targets: [" + n1->handle() + ", " + n2->handle() + "], "
            "is_toplevel: false, "
            "custom_attributes: {count: 10, flag: true}"
        ")"
    );
}

TEST(WildcardTest, Wildcards) {
    TestDecoder db;
    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;
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

    EXPECT_NO_THROW(v1.to_string());
    EXPECT_NO_THROW(v2.to_string());
    EXPECT_NO_THROW(v3.to_string());
    EXPECT_NO_THROW(v4.to_string());

    EXPECT_EQ(v1.schema_handle(), Atom::WILDCARD_STRING);

    LinkSchema schema(expression, 3);
    schema.stack_untyped_variable("v1");
    schema.stack_node(symbol, "n1");
    schema.stack_link_schema(expression, 2);
    schema.stack_untyped_variable("v2");
    schema.stack_node(symbol, "n2");
    schema.build();
    EXPECT_EQ(schema.metta_representation(db), "(($v1 n1) $v2 n2)");

    UntypedVariable v5("v5");
    Assignment assignment;
    EXPECT_TRUE(v5.match("h1", assignment, db));
    EXPECT_TRUE(v5.match("h1", assignment, db));
    EXPECT_FALSE(v5.match("h2", assignment, db));
    UntypedVariable v6("v6");
    EXPECT_TRUE(v6.match("h1", assignment, db));
}

TEST(WildcardTest, LinkSchema) {
    TestDecoder db;
    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;

    cout << "1 ---------------------------------------------------------" << endl;
    LinkSchema schema1(expression, 2);
    schema1.stack_node(symbol, "n1");
    schema1.stack_node(symbol, "n2");
    EXPECT_THROW(schema1.build(), runtime_error);

    // add metta_expression of mal-formed linkTemplate (type != expression)

    cout << "2 ---------------------------------------------------------" << endl;
    LinkSchema schema2(expression, 2);
    schema2.stack_node(symbol, "n1");
    schema2.stack_untyped_variable("v1");
    schema2.build();
    EXPECT_THROW(schema2.build(), runtime_error);
    EXPECT_THROW(schema2.stack_node(symbol, "n2"), runtime_error);
    EXPECT_EQ(schema2.metta_representation(db), "(n1 $v1)");
    LinkSchema schema2_1(schema2.tokenize());
    EXPECT_EQ(schema2.to_string(), schema2_1.to_string());
    EXPECT_TRUE(schema2 == schema2_1);

    cout << "3 ---------------------------------------------------------" << endl;
    LinkSchema schema3(expression, 3);
    schema3.stack_untyped_variable("v1");
    EXPECT_THROW(schema3.stack_link_schema(expression, 2), runtime_error);
    schema3.stack_node(symbol, "n1");
    schema3.stack_link_schema(expression, 2);
    schema3.stack_untyped_variable("v2");
    EXPECT_THROW(schema3.build(), runtime_error);
    schema3.stack_node(symbol, "n2");
    schema3.build();
    EXPECT_EQ(schema3.metta_representation(db), "(($v1 n1) $v2 n2)");
    LinkSchema schema3_1(schema3.tokenize());
    EXPECT_EQ(schema3.to_string(), schema3_1.to_string());
    EXPECT_TRUE(schema3 == schema3_1);

    cout << "4 ---------------------------------------------------------" << endl;
    LinkSchema schema4(expression, 2);
    schema4.stack_untyped_variable("v1");
    schema4.stack_untyped_variable("v2");
    schema4.stack_node(symbol, "n1");
    schema4.stack_link_schema(expression, 2);
    schema4.build();
    EXPECT_EQ(schema4.metta_representation(db), "($v1 ($v2 n1))");
    LinkSchema schema4_1(schema4.tokenize());
    EXPECT_EQ(schema4.to_string(), schema4_1.to_string());
    EXPECT_TRUE(schema4 == schema4_1);

    cout << "5 ---------------------------------------------------------" << endl;
    LinkSchema schema5(expression, 4);
    schema5.stack_untyped_variable("v1");
    schema5.stack_node(symbol, "n1");
    schema5.stack_link_schema(expression, 2);
    schema5.stack_untyped_variable("v2");
    schema5.stack_node(symbol, "n1");
    schema5.stack_node(symbol, "n2");
    schema5.stack_node(symbol, "n3");
    schema5.stack_node(symbol, "n6");
    schema5.stack_untyped_variable("v3");
    schema5.stack_link_schema(expression, 2);
    schema5.stack_node(symbol, "n5");
    schema5.stack_node(symbol, "n6");
    schema5.stack_link(expression, 2);
    schema5.stack_link_schema(expression, 3);
    schema5.stack_link_schema(expression, 3);
    schema5.stack_node(symbol, "n7");
    schema5.stack_node(symbol, "n8");
    schema5.stack_node(symbol, "n9");
    schema5.stack_node(symbol, "n10");
    schema5.stack_link(expression, 2);
    schema5.stack_link(expression, 2);
    schema5.stack_node(symbol, "n11");
    schema5.stack_link(expression, 3);
    schema5.build();
    // clang-format off
    EXPECT_EQ(schema5.metta_representation(db),
              "(($v1 n1) $v2 (n1 n2 (n3 (n6 $v3) (n5 n6))) (n7 (n8 (n9 n10)) n11))");
    // LINK_TEMPLATE Expression 4
    //     LINK_TEMPLATE Expression 2
    //         VARIABLE v1
    //         NODE Symbol n1
    //     VARIABLE v2
    //     LINK_TEMPLATE Expression 3
    //         NODE Symbol n1
    //         NODE Symbol n2
    //         LINK_TEMPLATE Expression 3
    //             NODE Symbol n3
    //             LINK_TEMPLATE Expression 2
    //                 NODE Symbol n6
    //                 VARIABLE v3
    //             LINK Expression 2
    //                 NODE Symbol n5
    //                 NODE Symbol n6
    //     LINK Expression 3
    //         NODE Symbol n7
    //         LINK Expression 2
    //             NODE Symbol n8
    //             LINK Expression 2
    //                 NODE Symbol n9
    //                 NODE Symbol n10
    //         NODE Symbol n11
    // clang-format on
    LinkSchema schema5_1(schema5.tokenize());
    EXPECT_EQ(schema5.to_string(), schema5_1.to_string());
    LinkSchema schema5_2(schema5);
    LinkSchema schema5_3 = schema5;
    EXPECT_TRUE(schema5 == schema5_1);
    EXPECT_TRUE(schema5 == schema5_2);
    EXPECT_TRUE(schema5 == schema5_3);

    cout << "6 ---------------------------------------------------------" << endl;
    LinkSchema schema6(expression, 2);
    schema6.stack_node(symbol, "n7");
    schema6.stack_node(symbol, "n8");
    schema6.stack_node(symbol, "n9");
    schema6.stack_node(symbol, "n10");
    schema6.stack_link(expression, 2);
    schema6.stack_link(expression, 2);
    schema6.stack_node(symbol, "n11");
    EXPECT_THROW(schema6.build(), runtime_error);

    cout << "7 ---------------------------------------------------------" << endl;
    LinkSchema schema7(expression, 2);
    schema7.stack_node(symbol, "n1");
    schema7.stack_untyped_variable("v1");
    EXPECT_THROW(schema7.stack_link(expression, 2), runtime_error);

    cout << "8 ---------------------------------------------------------" << endl;
    LinkSchema schema8(expression, 2);
    schema8.stack_atom("h1");
    schema8.stack_untyped_variable("v1");
    schema8.build();
    EXPECT_EQ(schema8.metta_representation(db), "(h1 $v1)");
    LinkSchema schema8_1(schema8.tokenize());
    EXPECT_EQ(schema8.to_string(), schema8_1.to_string());
    EXPECT_TRUE(schema8 == schema8_1);
}

void check_match(const string& test_tag,
                 LinkSchema& link_schema,
                 const vector<shared_ptr<Atom>>& targets,
                 TestDecoder& db,
                 bool test_flag) {
    LOG_INFO(test_tag);
    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;
    vector<string> v;
    Assignment assignment;

    for (auto target : targets) {
        v.push_back(target->handle());
    }
    auto link = db.add_atom(make_shared<Link>(expression, v));
    EXPECT_EQ(link_schema.match(*((Link*) (link.get())), assignment, db), test_flag);
}

TEST(LinkTest, Match) {
    TestDecoder db;
    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;
    vector<string> v;

    auto node1 = db.add_atom(make_shared<Node>(symbol, string("n1")));
    auto node2 = db.add_atom(make_shared<Node>(symbol, string("n2")));
    auto node3 = db.add_atom(make_shared<Node>(symbol, string("n3")));
    auto node4 = db.add_atom(make_shared<Node>(symbol, string("n4")));
    v = {node2->handle(), node1->handle()};
    auto link1 = db.add_atom(make_shared<Link>(expression, v));
    v = {node1->handle(), node2->handle()};
    auto link2 = db.add_atom(make_shared<Link>(expression, v));
    v = {node2->handle(), node3->handle(), node3->handle()};
    auto link3 = db.add_atom(make_shared<Link>(expression, v));
    v = {node1->handle(), link3->handle()};
    auto link4 = db.add_atom(make_shared<Link>(expression, v));

    Assignment assignment;
    EXPECT_TRUE(node1->match(node1->handle(), assignment, db));
    EXPECT_FALSE(node1->match(node2->handle(), assignment, db));
    EXPECT_TRUE(link1->match(link1->handle(), assignment, db));
    EXPECT_FALSE(link1->match(node1->handle(), assignment, db));

    // clang-format off
    LinkSchema schema1({
    "LINK_TEMPLATE", "Expression", "2",
        "VARIABLE", "v2",
        "NODE", "Symbol", "n2"});
    check_match("test case 1.1", schema1, {node1, node2}, db, true);
    check_match("test case 1.2", schema1, {node2, node1}, db, false);

    LinkSchema schema2({
    "LINK_TEMPLATE", "Expression", "3",
        "LINK_TEMPLATE", "Expression", "2",
            "VARIABLE", "v1",
            "NODE", "Symbol", "n1",
        "VARIABLE", "v2",
        "NODE", "Symbol", "n2"});
    check_match("test case 2.1", schema2, {node3, node1, node2}, db, false);
    check_match("test case 2.2", schema2, {node2, node2, node2}, db, false);
    check_match("test case 2.3", schema2, {node1, node1}, db, false);
    check_match("test case 2.4", schema2, {node1, node2}, db, false);
    check_match("test case 2.5", schema2, {link1, node1, node2}, db, true);
    check_match("test case 2.6", schema2, {link1, node4, node2}, db, true);
    check_match("test case 2.7", schema2, {link1, node1, node1}, db, false);
    check_match("test case 2.5", schema2, {link2, node1, node2}, db, false);

    LinkSchema schema3({
    "LINK_TEMPLATE", "Expression", "3",
        "VARIABLE", "v2",
        "NODE", "Symbol", "n2",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "n1",
            "VARIABLE", "v1"});
    check_match("test case 3.1", schema3, {node1, node2, node3}, db, false);
    check_match("test case 3.2", schema3, {node1, node2, link1}, db, false);
    check_match("test case 3.3", schema3, {node1, node2, link2}, db, true);

    LinkSchema schema4({
    "LINK_TEMPLATE", "Expression", "3",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "n1",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "n2",
                "NODE", "Symbol", "n3",
                "VARIABLE", "v1",
        "NODE", "Symbol", "n2",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "n1",
            "VARIABLE", "v2",
            });
    check_match("test case 4.1", schema4, {link4, node2, link2}, db, true);

    LinkSchema schema5({
    "LINK_TEMPLATE", "Expression", "3",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "n1",
            "LINK_TEMPLATE", "Expression", "3",
                "ATOM", Hasher::node_handle("Symbol", "n2"),
                "ATOM", Hasher::node_handle("Symbol", "n3"),
                "VARIABLE", "v1",
        "ATOM", Hasher::node_handle("Symbol", "n2"),
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "n1",
            "VARIABLE", "v2",
            });
    check_match("test case 5.1", schema5, {link4, node2, link2}, db, true);

    // clang-format on
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
    //cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
    //cout << "node1: " << node1->handle() << endl;
    //cout << "node2: " << node2->handle() << endl;
    //cout << "node3: " << node3->handle() << endl;
    //cout << "node4: " << node4->handle() << endl;
    //cout << "Expression: " << expression_hash << " Symbol: " << symbol_hash << endl;
    //cout << "----------------------------------------------" << endl;
    //cout << "link1: " << link1->handle() << " " << link1->to_string() << endl;
    //cout << link1_composite.size() << " " << link1_composite[0] << " " << link1_composite[1] << " " << link1_composite[2] << endl;
    //cout << link1_composite_hash << endl;
    //cout << "----------------------------------------------" << endl;
    //cout << "link2: " << link2->handle() << " " << link2->to_string() << endl;
    //cout << link2_composite.size() << " " << link2_composite[0] << " " << link2_composite[1] << " " << link2_composite[2] << endl;
    //cout << link2_composite_hash << endl;
    //cout << "----------------------------------------------" << endl;
    //cout << "link3: " << link3->handle() << " " << link3->to_string() << endl;
    //cout << link3_composite.size() << " " << link3_composite[0] << " " << link3_composite[1] << " " << link3_composite[2] << endl;
    //cout << link3_composite_hash << endl;
    //cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
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
