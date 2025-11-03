#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <thread>

#include "AtomDBSingleton.h"
#include "ImplicationProcessor.h"
#include "LinkCreateTemplate.h"
#include "LinkCreationAgent.h"
#include "LinkCreationRequestProcessor.h"
#include "MockAtomDB.h"
#include "MockServiceBus.h"
#include "TemplateProcessor.h"
#include "Utils.h"

using namespace std;
using namespace link_creation_agent;
using namespace commons;
using namespace service_bus;
using namespace atomdb;

class MockLinkCreationRequestProxy : public LinkCreationRequestProxy {
   public:
    MockLinkCreationRequestProxy(const vector<string>& tokens) : LinkCreationRequestProxy(tokens) {}
    ~MockLinkCreationRequestProxy() {}
    MOCK_METHOD(string, peer_id, (), (override));
    MOCK_METHOD(uint, get_serial, (), (override));
};

class LinkCreationAgentTest : public ::testing::Test {
   protected:
    int request_interval;
    int thread_count;
    int default_timeout;
    string buffer_file_path;
    string metta_file_path;
    bool save_links_to_metta_file = true;
    bool save_links_to_db = false;
    string server_id;

    void SetUp() override {
        this->request_interval = 1;
        this->thread_count = 1;
        this->default_timeout = 10;
        this->buffer_file_path = "test_buffer.bin";
        this->metta_file_path = ".";
        this->save_links_to_metta_file = true;
        this->save_links_to_db = false;
        this->server_id = "localhost:40040";
        AtomDBSingleton::provide(move(make_shared<AtomDBMock>()));
        ServiceBusSingleton::provide(
            move(make_shared<MockServiceBus>("localhost:40038", "localhost:40039")));
    }

    void TearDown() override {
        remove("test_buffer.bin");
        AtomDBSingleton::provide(nullptr);
        ServiceBusSingleton::provide(nullptr);
    }
};

TEST_F(LinkCreationAgentTest, TestRequest) {
    auto agent = new LinkCreationAgent();
    vector<string> request = {
        "query1",
        "LINK_CREATE",
        "test",
        "1",
        "0",
        "VARIABLE",
        "V1",
    };
    auto proxy = make_shared<MockLinkCreationRequestProxy>(request);
    proxy->parameters[LinkCreationRequestProxy::MAX_ANSWERS] = (uint) 10;
    proxy->parameters[LinkCreationRequestProxy::REPEAT_COUNT] = (uint) 5;
    proxy->parameters[LinkCreationRequestProxy::CONTEXT] = "test_context";
    proxy->parameters[LinkCreationRequestProxy::ATTENTION_UPDATE_FLAG] = true;

    shared_ptr<LinkCreationAgentRequest> lca_request = agent->create_request(proxy);
    EXPECT_EQ(lca_request->query, vector<string>({"query1"}));
    EXPECT_EQ(lca_request->link_template,
              vector<string>({"LINK_CREATE", "test", "1", "0", "VARIABLE", "V1"}));
    EXPECT_EQ(lca_request->max_results, 10);
    EXPECT_EQ(lca_request->repeat, 5);
    EXPECT_EQ(lca_request->context, "test_context");
    EXPECT_EQ(lca_request->update_attention_broker, true);
    EXPECT_EQ(lca_request->infinite, false);
    EXPECT_EQ(lca_request->id.empty(), false);
    request.clear();
    request = {"query2", "LINK_CREATE", "test2", "2", "1", "NODE", "Symbol", "A", "VARIABLE", "V1"};
    proxy = make_shared<MockLinkCreationRequestProxy>(request);
    proxy->parameters[LinkCreationRequestProxy::MAX_ANSWERS] = (uint) 10;
    proxy->parameters[LinkCreationRequestProxy::REPEAT_COUNT] = (uint) 0;
    proxy->parameters[LinkCreationRequestProxy::CONTEXT] = "test_context";
    proxy->parameters[LinkCreationRequestProxy::ATTENTION_UPDATE_FLAG] = false;
    lca_request = agent->create_request(proxy);

    EXPECT_EQ(lca_request->query, vector<string>({"query2"}));
    EXPECT_EQ(
        lca_request->link_template,
        vector<string>({"LINK_CREATE", "test2", "2", "1", "NODE", "Symbol", "A", "VARIABLE", "V1"}));
    EXPECT_EQ(lca_request->max_results, 10);
    EXPECT_EQ(lca_request->repeat, 0);
    EXPECT_EQ(lca_request->context, "test_context");
    EXPECT_EQ(lca_request->update_attention_broker, false);
    EXPECT_EQ(lca_request->infinite, true);
    EXPECT_EQ(lca_request->id, "cfcd208495d565ef66e7dff9f98764da");
    delete agent;
}

TEST_F(LinkCreationAgentTest, TestRequestMetta) {
    auto agent = new LinkCreationAgent();
    vector<string> request = {"()", "()"};
    auto proxy = make_shared<MockLinkCreationRequestProxy>(request);
    proxy->parameters[LinkCreationRequestProxy::MAX_ANSWERS] = (uint) 10;
    proxy->parameters[LinkCreationRequestProxy::REPEAT_COUNT] = (uint) 5;
    proxy->parameters[LinkCreationRequestProxy::CONTEXT] = "test_context";
    proxy->parameters[LinkCreationRequestProxy::ATTENTION_UPDATE_FLAG] = true;
    proxy->parameters[LinkCreationRequestProxy::USE_METTA_AS_QUERY_TOKENS] = true;

    shared_ptr<LinkCreationAgentRequest> lca_request = agent->create_request(proxy);
    EXPECT_EQ(lca_request->query, vector<string>({"()"}));
    EXPECT_EQ(lca_request->link_template, vector<string>({"METTA", "()"}));
    EXPECT_EQ(lca_request->max_results, 10);
    EXPECT_EQ(lca_request->repeat, 5);
    EXPECT_EQ(lca_request->context, "test_context");
    EXPECT_EQ(lca_request->update_attention_broker, true);
    EXPECT_EQ(lca_request->infinite, false);
    EXPECT_EQ(lca_request->id.empty(), false);
    request.clear();
    delete agent;
}

TEST_F(LinkCreationAgentTest, TestConfig) {
    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(
        make_shared<LinkCreationRequestProcessor>(this->request_interval,
                                                  this->thread_count,
                                                  this->default_timeout,
                                                  this->buffer_file_path,
                                                  this->metta_file_path,
                                                  this->save_links_to_metta_file,
                                                  this->save_links_to_db,
                                                  false));
}

TEST(LinkCreateTemplate, TestCustomField) {
    vector<string> args = {"CUSTOM_FIELD", "field1", "2", "N1", "value1", "N2", "value2"};
    CustomField custom_field(args);
    EXPECT_EQ(custom_field.get_name(), "field1");
    vector<tuple<string, CustomFieldTypes>> values = custom_field.get_values();
    EXPECT_EQ(values.size(), 2);
    EXPECT_EQ(get<string>(get<1>(values[0])), "value1");
    EXPECT_EQ(get<string>(get<1>(values[1])), "value2");

    // Test custom field with no values
    vector<string> args_empty = {"CUSTOM_FIELD", "empty_field", "0"};
    CustomField custom_field_empty(args_empty);
    EXPECT_EQ(custom_field_empty.get_name(), "empty_field");
    EXPECT_EQ(custom_field_empty.get_values().size(), 0);

    // Test custom field with multiple values
    vector<string> args_multiple = {
        "CUSTOM_FIELD", "multi_field", "3", "N1", "value1", "N2", "value2", "N3", "value3"};
    CustomField custom_field_multiple(args_multiple);
    EXPECT_EQ(custom_field_multiple.get_name(), "multi_field");
    vector<tuple<string, CustomFieldTypes>> values_multiple = custom_field_multiple.get_values();
    EXPECT_EQ(values_multiple.size(), 3);
    EXPECT_EQ(get<string>(get<1>(values_multiple[0])), "value1");
    EXPECT_EQ(get<string>(get<1>(values_multiple[1])), "value2");
    EXPECT_EQ(get<string>(get<1>(values_multiple[2])), "value3");

    // Test nested custom fields
    vector<string> args_nested = {"CUSTOM_FIELD",
                                  "nested_field",
                                  "2",
                                  "N1",
                                  "value1",
                                  "CUSTOM_FIELD",
                                  "inner_field",
                                  "1",
                                  "N2",
                                  "value2"};
    CustomField custom_field_nested(args_nested);
    EXPECT_EQ(custom_field_nested.get_name(), "nested_field");
    vector<tuple<string, CustomFieldTypes>> values_nested = custom_field_nested.get_values();
    EXPECT_EQ(values_nested.size(), 2);
    EXPECT_EQ(get<string>(get<1>(values_nested[0])), "value1");
    shared_ptr<CustomField> inner_field = get<shared_ptr<CustomField>>(get<1>(values_nested[1]));
    EXPECT_EQ(inner_field->get_name(), "inner_field");
    vector<tuple<string, CustomFieldTypes>> inner_values = inner_field->get_values();
    EXPECT_EQ(inner_values.size(), 1);
    EXPECT_EQ(get<string>(get<1>(inner_values[0])), "value2");
}

vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

TEST(LinkCreateTemplate, TestLinkCreateTemplate) {
    /**  #NOTE Different from the original string test, the to_string() method is not returning the
    same
       order as the input string. to_string is placing the custom fields after the targets
    */
    string link_template_str =
        "LINK_CREATE Similarity 3 1 VARIABLE V1 LINK_CREATE Test 3 0 NODE Symbol A VARIABLE V2 "
        "LINK_CREATE Test2 1 1 NODE Symbol C CUSTOM_FIELD inter 1 inter_name inter_value NODE Symbol B "
        "CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9";
    auto link_template = Utils::split(link_template_str, ' ');
    LinkCreateTemplate lct(link_template);
    EXPECT_EQ(lct.get_link_type(), "Similarity");
    EXPECT_EQ(lct.to_string(), link_template_str);
    link_template.clear();
    link_template_str.clear();

    link_template_str =
        "LINK_CREATE I 3 0 VARIABLE V1 LINK_CREATE Test 3 0 NODE Symbol A VARIABLE V2 LINK_CREATE Test2 "
        "1 0 NODE Symbol C NODE Symbol B";
    link_template = split(link_template_str, ' ');
    LinkCreateTemplate lct2(link_template);
    EXPECT_EQ(lct2.get_link_type(), "I");
    EXPECT_EQ(lct2.to_string(), link_template_str);
    EXPECT_EQ(lct2.get_targets().size(), 3);
    EXPECT_EQ(get<shared_ptr<UntypedVariable>>(lct.get_targets()[0])->name, "V1");
    link_template.clear();
    link_template_str.clear();

    link_template_str =
        "LINK_CREATE Similarity 2 1 VARIABLE V1 VARIABLE V2 CUSTOM_FIELD truth_value 2 CUSTOM_FIELD "
        "mean 2 count 10 avg 0.9 confidence 0.9";
    link_template = split(link_template_str, ' ');
    LinkCreateTemplate lct3(link_template);
    EXPECT_EQ(lct3.get_link_type(), "Similarity");
    EXPECT_EQ(lct3.to_string(), link_template_str);
    link_template.clear();
    link_template_str.clear();

    link_template_str =
        "LINK_CREATE link_type 2 1 NODE type1 value1 VARIABLE var1 CUSTOM_FIELD field1 1 value1 value2";
    link_template = split(link_template_str, ' ');
    LinkCreateTemplate lct4(link_template);
    EXPECT_EQ(lct4.get_link_type(), "link_type");
    EXPECT_EQ(lct4.to_string(), link_template_str);
    link_template.clear();
    link_template_str.clear();

    link_template_str =
        "LINK_CREATE TestLink 4 1 NODE NodeType1 Value1 VARIABLE Var1 CUSTOM_FIELD Field1 2 valuename1 "
        "Value1 valuename2 Value2 NODE NodeType2 Value2 VARIABLE Var2";
    link_template = split(link_template_str, ' ');
    LinkCreateTemplate lct5(link_template);
    EXPECT_EQ(lct5.get_link_type(), "TestLink");
    EXPECT_EQ(lct5.to_string(),
              "LINK_CREATE TestLink 4 1 NODE NodeType1 Value1 VARIABLE Var1 NODE NodeType2 Value2 "
              "VARIABLE Var2 CUSTOM_FIELD Field1 2 valuename1 Value1 valuename2 Value2");
    EXPECT_EQ(lct5.get_targets().size(), 4);
    EXPECT_EQ(get<shared_ptr<Node>>(lct5.get_targets()[0])->type, "NodeType1");
    EXPECT_EQ(get<shared_ptr<Node>>(lct5.get_targets()[0])->name, "Value1");
    EXPECT_EQ(get<shared_ptr<UntypedVariable>>(lct5.get_targets()[1])->name, "Var1");
    link_template.clear();
    link_template_str.clear();

    link_template_str =
        "LINK_CREATE AnotherLink 2 1 VARIABLE VarA NODE NodeTypeA ValueA CUSTOM_FIELD FieldA 1 NameA "
        "ValueA";
    link_template = split(link_template_str, ' ');
    LinkCreateTemplate lct6(link_template);
    EXPECT_EQ(lct6.get_link_type(), "AnotherLink");
    EXPECT_EQ(lct6.to_string(), link_template_str);
    EXPECT_EQ(lct6.get_targets().size(), 2);
    EXPECT_EQ(get<shared_ptr<UntypedVariable>>(lct6.get_targets()[0])->name, "VarA");
    EXPECT_EQ(get<shared_ptr<Node>>(lct6.get_targets()[1])->type, "NodeTypeA");
    EXPECT_EQ(get<shared_ptr<Node>>(lct6.get_targets()[1])->name, "ValueA");
    EXPECT_EQ(lct6.get_custom_fields().size(), 1);
    EXPECT_EQ(lct6.get_custom_fields()[0].get_name(), "FieldA");
    EXPECT_EQ(lct6.get_custom_fields()[0].get_values().size(), 1);
    EXPECT_EQ(get<0>(lct6.get_custom_fields()[0].get_values()[0]), "NameA");
    EXPECT_EQ(get<string>(get<1>(lct6.get_custom_fields()[0].get_values()[0])), "ValueA");
    link_template.clear();
    link_template_str.clear();

    link_template_str =
        "LINK_CREATE ComplexLink 4 2 NODE Type1 Val1 VARIABLE Var1 CUSTOM_FIELD Field1 2 N1 Val1 N2 "
        "Val2 NODE Type2 Val2 VARIABLE Var2 CUSTOM_FIELD Field2 1 N3 Val3";
    link_template = split(link_template_str, ' ');
    LinkCreateTemplate lct7(link_template);
    EXPECT_EQ(lct7.get_link_type(), "ComplexLink");
    EXPECT_EQ(lct7.to_string(),
              "LINK_CREATE ComplexLink 4 2 NODE Type1 Val1 VARIABLE Var1 NODE Type2 Val2 VARIABLE Var2 "
              "CUSTOM_FIELD Field1 2 N1 Val1 N2 Val2 CUSTOM_FIELD Field2 1 N3 Val3");
    EXPECT_EQ(lct7.get_targets().size(), 4);
    EXPECT_EQ(get<shared_ptr<Node>>(lct7.get_targets()[0])->type, "Type1");
    EXPECT_EQ(get<shared_ptr<Node>>(lct7.get_targets()[0])->name, "Val1");
    EXPECT_EQ(get<shared_ptr<UntypedVariable>>(lct7.get_targets()[1])->name, "Var1");
    EXPECT_EQ(get<shared_ptr<Node>>(lct7.get_targets()[2])->type, "Type2");
    EXPECT_EQ(get<shared_ptr<Node>>(lct7.get_targets()[2])->name, "Val2");
    EXPECT_EQ(get<shared_ptr<UntypedVariable>>(lct7.get_targets()[3])->name, "Var2");
    EXPECT_EQ(lct7.get_custom_fields()[0].get_name(), "Field1");
    EXPECT_EQ(lct7.get_custom_fields()[0].get_values().size(), 2);
    EXPECT_EQ(get<0>(lct7.get_custom_fields()[0].get_values()[0]), "N1");
    EXPECT_EQ(get<string>(get<1>(lct7.get_custom_fields()[0].get_values()[0])), "Val1");
    EXPECT_EQ(get<0>(lct7.get_custom_fields()[0].get_values()[1]), "N2");
    EXPECT_EQ(get<string>(get<1>(lct7.get_custom_fields()[0].get_values()[1])), "Val2");
    EXPECT_EQ(lct7.get_custom_fields()[1].get_name(), "Field2");
    EXPECT_EQ(lct7.get_custom_fields()[1].get_values().size(), 1);
    EXPECT_EQ(get<0>(lct7.get_custom_fields()[1].get_values()[0]), "N3");
    EXPECT_EQ(get<string>(get<1>(lct7.get_custom_fields()[1].get_values()[0])), "Val3");
    link_template.clear();
    link_template_str.clear();

    link_template_str = "LINK_CREATE SimpleLink 2 0 VARIABLE SimpleVar NODE SimpleNode SimpleValue";
    link_template = split(link_template_str, ' ');
    LinkCreateTemplate lct8(link_template);
    EXPECT_EQ(lct8.get_link_type(), "SimpleLink");
    EXPECT_EQ(lct8.to_string(), link_template_str);
    EXPECT_EQ(lct8.get_targets().size(), 2);
    EXPECT_EQ(get<shared_ptr<UntypedVariable>>(lct8.get_targets()[0])->name, "SimpleVar");
    EXPECT_EQ(get<shared_ptr<Node>>(lct8.get_targets()[1])->type, "SimpleNode");
    EXPECT_EQ(get<shared_ptr<Node>>(lct8.get_targets()[1])->name, "SimpleValue");
    link_template.clear();
    link_template_str.clear();
}

TEST(LinkCreateTemplate, TestInvalidLinkType) {
    string link_template_str = "LINK_CREATE 3 1 VARIABLE V1 NODE Symbol A";
    auto link_template = split(link_template_str, ' ');
    EXPECT_THROW(LinkCreateTemplate lct(link_template), invalid_argument);
}

TEST(LinkCreateTemplate, TestInvalidTargetCount) {
    string link_template_str = "LINK_CREATE Similarity 3 1 VARIABLE V1 NODE Symbol A";
    auto link_template = split(link_template_str, ' ');
    EXPECT_THROW(LinkCreateTemplate lct(link_template), runtime_error);
}

TEST(LinkCreateTemplate, TestInvalidCustomField) {
    string link_template_str =
        "LINK_CREATE Similarity 2 1 VARIABLE V1 NODE Symbol A CUSTOM_FIELD field1";
    auto link_template = split(link_template_str, ' ');
    EXPECT_THROW(LinkCreateTemplate lct(link_template), runtime_error);
}

TEST(LinkCreateTemplate, TestInvalidVariable) {
    string link_template_str = "LINK_CREATE Similarity 2 1 VARIABLE NODE Symbol A";
    auto link_template = split(link_template_str, ' ');
    EXPECT_THROW(LinkCreateTemplate lct(link_template), runtime_error);
}

TEST(LinkCreateTemplate, TestInvalidNode) {
    string link_template_str = "LINK_CREATE Similarity 2 1 VARIABLE V1 NODE Symbol";
    auto link_template = split(link_template_str, ' ');
    EXPECT_THROW(LinkCreateTemplate lct(link_template), runtime_error);
}

TEST_F(LinkCreationAgentTest, TestLinkTemplateProcessor) {
    vector<string> link_template = split("LINK_CREATE Expression 2 0 VARIABLE V1 VARIABLE V2", ' ');
    shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>();
    query_answer->assignment.assign("V1", "Value1");
    query_answer->assignment.assign("V2", "Value2");

    LinkTemplateProcessor ltp;
    auto links = ltp.process_query(query_answer, link_template);
    auto link2 = LinkCreateTemplate(link_template).process_query_answer(query_answer);
    cout << link2->to_string() << endl;
    EXPECT_EQ(link2->to_string(),
              "Link(type: 'Expression', targets: [Value1, Value2], is_toplevel: false, "
              "custom_attributes: {})");
    EXPECT_EQ(links[0]->to_string(),
              "Link(type: 'Expression', targets: [Value1, Value2], is_toplevel: false, "
              "custom_attributes: {})");
    auto mock_atom = dynamic_cast<AtomDBMock*>(AtomDBSingleton::get_instance().get());
    vector<string> targets_node = {"Value1", "Value2", "A", "Value1", "B", "C", "B"};
    EXPECT_CALL(*mock_atom, get_atom(testing::_))
        .Times(targets_node.size())
        .WillRepeatedly(::testing::Invoke([&targets_node](const string& handle) {
            auto node = make_shared<Node>("Symbol", targets_node.front());
            targets_node.erase(targets_node.begin());
            return node;
        }));
    EXPECT_EQ(links[0]->metta_representation(*AtomDBSingleton::get_instance().get()), "(Value1 Value2)");
    link_template.clear();
    links.clear();

    query_answer = make_shared<QueryAnswer>();
    link_template = split("LINK_CREATE Expression 3 0 NODE Symbol A VARIABLE V1 NODE Symbol B", ' ');
    query_answer->assignment.assign("V1", "Value1");
    links = ltp.process_query(query_answer, link_template);
    EXPECT_EQ(links[0]->to_string(),
              "Link(type: 'Expression', targets: [11944827f32b09c425b6136a6b5b4224, Value1, "
              "d4891853e7729b52d422daa93ccecacb], is_toplevel: false, custom_attributes: {})");
    EXPECT_EQ(links[0]->metta_representation(*AtomDBSingleton::get_instance().get()), "(A Value1 B)");
    links.clear();
    link_template.clear();

    query_answer = make_shared<QueryAnswer>();
    link_template = split(
        "LINK_CREATE Expression 2 1 NODE Symbol C NODE Symbol B "
        "CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9",
        ' ');
    links = ltp.process_query(query_answer, link_template);
    EXPECT_EQ(links[0]->to_string(),
              "Link(type: 'Expression', targets: [9908489fa1968f547004d4d56dc700bb, "
              "d4891853e7729b52d422daa93ccecacb], is_toplevel: false, custom_attributes: {confidence: "
              "'0.9', mean.avg: "
              "'0.9', mean.count: '10'})");
    EXPECT_EQ(links[0]->metta_representation(*AtomDBSingleton::get_instance().get()), "(C B)");
    links.clear();
    link_template.clear();

    query_answer = make_shared<QueryAnswer>();
    link_template = split(
        "LINK_CREATE Test3 2 1 VARIABLE V1 VARIABLE V2 "
        "CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9",
        ' ');
    query_answer->assignment.assign("V1", "Value1");
    query_answer->assignment.assign("V2", "Value2");
    links = ltp.process_query(query_answer, link_template);
    EXPECT_EQ(links[0]->to_string(),
              "Link(type: 'Test3', targets: [Value1, Value2], is_toplevel: false, custom_attributes: "
              "{confidence: '0.9', "
              "mean.avg: '0.9', mean.count: '10'})");
    link_template.clear();
    links.clear();
    ASSERT_TRUE(targets_node.empty());
}

TEST(ImplicationProcessor, TestImplicationProcessorQueryBuilding) {
    LinkSchema ls = ImplicationProcessor::build_pattern_query("HANDLE Value1");
    vector<string> output;
    ls.tokenize(output);
    EXPECT_EQ(Utils::join(output, ' '),
              "LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION LINK Expression 2 NODE Symbol "
              "PREDICATE ATOM HANDLE Value1 LINK_TEMPLATE Expression 2 NODE Symbol CONCEPT VARIABLE PX");
    output.clear();
    ls = ImplicationProcessor::build_satisfying_set_query("h1", "h2");
    ls.tokenize(output);
    EXPECT_EQ(Utils::join(output, ' '),
              "LINK_TEMPLATE AND 2 LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION LINK Expression 2 "
              "NODE Symbol PREDICATE ATOM h1 LINK_TEMPLATE Expression 2 NODE Symbol CONCEPT VARIABLE C "
              "LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION LINK Expression 2 NODE Symbol "
              "PREDICATE ATOM h2 LINK_TEMPLATE Expression 2 NODE Symbol CONCEPT VARIABLE C");
}

TEST(EquivalenceProcessor, TestEquivalenceProcessorQueryBuilding) {
    LinkSchema ls = LinkProcessor::build_pattern_set_query("h1", "h2");
    vector<string> output;
    ls.tokenize(output);
    EXPECT_EQ(Utils::join(output, ' '),
              "LINK_TEMPLATE AND 2 LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION LINK_TEMPLATE "
              "Expression 2 NODE Symbol PREDICATE VARIABLE P LINK Expression 2 NODE Symbol CONCEPT ATOM "
              "h1 LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION LINK_TEMPLATE Expression 2 NODE "
              "Symbol PREDICATE VARIABLE P LINK Expression 2 NODE Symbol CONCEPT ATOM h2");

    ls = LinkProcessor::build_pattern_union_query("h1", "h2");
    output.clear();
    ls.tokenize(output);
    EXPECT_EQ(Utils::join(output, ' '),
              "LINK_TEMPLATE OR 2 LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION LINK_TEMPLATE "
              "Expression 2 NODE Symbol PREDICATE VARIABLE P LINK Expression 2 NODE Symbol CONCEPT "
              "ATOM h1 LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION LINK_TEMPLATE Expression 2 "
              "NODE Symbol PREDICATE VARIABLE P LINK Expression 2 NODE Symbol CONCEPT ATOM h2");
}

TEST_F(LinkCreationAgentTest, TestImplicationProcessorLinkCreation) {
    auto mock_service_bus = dynamic_cast<MockServiceBus*>(ServiceBusSingleton::get_instance().get());
    string A = "A";
    string B = "B";
    string C = "C";
    vector<vector<string>> calls = {
        {A, B, C},
        {B, A},
    };

    vector<vector<pair<string, string>>> pairs = {{{A, B}, {B, C}, {C, A}}, {{B, A}, {C, B}}};
    int count = 0;
    EXPECT_CALL(*mock_service_bus, issue_bus_command(testing::_))
        .Times(2)
        .WillRepeatedly(
            ::testing::Invoke([&count, &calls, &pairs](shared_ptr<BusCommandProxy> command_proxy) {
                auto proxy = dynamic_pointer_cast<PatternMatchingQueryProxy>(command_proxy);
                vector<string> query_answers;
                int cc = 0;
                for (const auto& handle : calls[count]) {
                    auto qa = make_shared<QueryAnswer>(handle, 0.0);
                    qa->assignment.assign("C1", pairs[count][cc].first);
                    qa->assignment.assign("C2", pairs[count][cc].second);
                    cc++;
                    query_answers.push_back(qa->tokenize());
                }
                proxy->answer_bundle(query_answers);
                proxy->command_finished({});
                count++;
            }));
    auto ip = make_shared<ImplicationProcessor>();
    shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>(1.0);
    query_answer->add_handle(A);
    query_answer->add_handle(B);
    auto links = ip->process_query(query_answer, vector<string>({"context"}));
    EXPECT_EQ(links.size(), 2);
    EXPECT_NEAR(links[0]->custom_attributes.get<double>("strength"), 0.66, 1e-2);
    EXPECT_NEAR(links[1]->custom_attributes.get<double>("strength"), 1, 1e-2);
}

TEST_F(LinkCreationAgentTest, TestImplicationProcessorLinkCreationOr) {
    auto mock_service_bus = dynamic_cast<MockServiceBus*>(ServiceBusSingleton::get_instance().get());
    string A = "A";
    string B = "B";
    string C = "C";
    vector<vector<pair<string, string>>> calls = {{{A, B}, {B, C}, {C, A}}, {{B, A}, {C, B}}};

    vector<vector<pair<string, string>>> pairs = {{{A, B}, {B, C}, {C, A}}, {{B, A}, {C, B}}};
    int count = 0;
    EXPECT_CALL(*mock_service_bus, issue_bus_command(testing::_))
        .Times(2)
        .WillRepeatedly(
            ::testing::Invoke([&count, &calls, &pairs](shared_ptr<BusCommandProxy> command_proxy) {
                auto proxy = dynamic_pointer_cast<PatternMatchingQueryProxy>(command_proxy);
                vector<string> query_answers;
                int cc = 0;
                for (const auto& handle : calls[count]) {
                    auto qa = make_shared<QueryAnswer>(handle.first, 0.0);
                    qa->add_handle(handle.second);
                    qa->assignment.assign("C1", pairs[count][cc].first);
                    qa->assignment.assign("C2", pairs[count][cc].second);
                    cc++;
                    query_answers.push_back(qa->tokenize());
                }
                proxy->answer_bundle(query_answers);
                proxy->command_finished({});
                count++;
            }));

    auto targets = vector<pair<string, string>>{{A, B}, {B, A}, {B, C}, {C, B}, {C, A}, {A, C}};
    int t_count = 0;
    auto mock_atomdb = dynamic_cast<AtomDBMock*>(AtomDBSingleton::get_instance().get());
    EXPECT_CALL(*mock_atomdb, get_atom(testing::_))
        .Times(5)
        .WillRepeatedly(::testing::Invoke([&targets, &t_count](const string& handle) {
            Properties props;
            props["strength"] = 0.33 * (t_count + 1);
            vector<string> targets_ = {targets[t_count].first, targets[t_count].second};
            auto link = make_shared<Link>("Expression", targets_, props);
            t_count++;
            return link;
        }));
    auto ip = make_shared<ImplicationProcessor>();
    shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>(1.0);
    query_answer->add_handle(A);
    query_answer->add_handle(B);
    auto links = ip->process_query(query_answer, vector<string>({"context"}));
    EXPECT_EQ(links.size(), 2);
    // TODO review these expected values
    EXPECT_NEAR(links[0]->custom_attributes.get<double>("strength"), 1.5, 1e-2);
    EXPECT_NEAR(links[1]->custom_attributes.get<double>("strength"), 1, 1e-2);
}

TEST_F(LinkCreationAgentTest, TestEquivalenceProcessorLinkCreation) {
    auto mock_service_bus = dynamic_cast<MockServiceBus*>(ServiceBusSingleton::get_instance().get());
    string A = "A";
    string B = "B";
    string C = "C";
    string D = "D";
    vector<vector<string>> calls = {
        {A, B, C},
        {B, A, D},
    };

    vector<vector<pair<string, string>>> pairs = {{{A, B}, {B, C}, {C, A}}, {{B, A}, {C, B}, {D, C}}};
    int count = 0;
    EXPECT_CALL(*mock_service_bus, issue_bus_command(testing::_))
        .Times(2)
        .WillRepeatedly(
            ::testing::Invoke([&count, &calls, &pairs](shared_ptr<BusCommandProxy> command_proxy) {
                auto proxy = dynamic_pointer_cast<PatternMatchingQueryProxy>(command_proxy);
                vector<string> query_answers;
                int cc = 0;
                for (const auto& handle : calls[count]) {
                    auto qa = make_shared<QueryAnswer>(handle, 0.0);
                    qa->assignment.assign("P1", pairs[count][cc].first);
                    qa->assignment.assign("P2", pairs[count][cc].second);
                    cc++;
                    query_answers.push_back(qa->tokenize());
                }
                proxy->answer_bundle(query_answers);
                proxy->command_finished({});
                count++;
            }));
    auto ep = make_shared<EquivalenceProcessor>();
    shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>(1.0);
    query_answer->add_handle(A);
    query_answer->add_handle(B);
    auto links = ep->process_query(query_answer, vector<string>({"context"}));
    EXPECT_EQ(links.size(), 2);
    EXPECT_NEAR(links[0]->custom_attributes.get<double>("strength"), 0.5, 1e-2);
    EXPECT_NEAR(links[1]->custom_attributes.get<double>("strength"), 0.5, 1e-2);
}

TEST_F(LinkCreationAgentTest, TestEquivalenceProcessorLinkCreationOr) {
    auto mock_service_bus = dynamic_cast<MockServiceBus*>(ServiceBusSingleton::get_instance().get());
    string A = "A";
    string B = "B";
    string C = "C";
    vector<vector<pair<string, string>>> calls = {{{A, B}, {B, C}, {C, A}}, {{B, A}, {C, B}}};

    vector<vector<pair<string, string>>> pairs = {{{A, B}, {B, C}, {C, A}}, {{B, A}, {C, B}}};
    int count = 0;
    EXPECT_CALL(*mock_service_bus, issue_bus_command(testing::_))
        .Times(2)
        .WillRepeatedly(
            ::testing::Invoke([&count, &calls, &pairs](shared_ptr<BusCommandProxy> command_proxy) {
                auto proxy = dynamic_pointer_cast<PatternMatchingQueryProxy>(command_proxy);
                vector<string> query_answers;
                int cc = 0;
                for (const auto& handle : calls[count]) {
                    auto qa = make_shared<QueryAnswer>(handle.first, 0.0);
                    qa->add_handle(handle.second);
                    qa->assignment.assign("P1", pairs[count][cc].first);
                    qa->assignment.assign("P2", pairs[count][cc].second);
                    cc++;
                    query_answers.push_back(qa->tokenize());
                }
                proxy->answer_bundle(query_answers);
                proxy->command_finished({});
                count++;
            }));

    auto targets = vector<pair<string, string>>{{A, B}, {B, A}, {B, C}, {C, B}, {C, A}, {A, C}};
    int t_count = 0;
    auto mock_atomdb = dynamic_cast<AtomDBMock*>(AtomDBSingleton::get_instance().get());
    EXPECT_CALL(*mock_atomdb, get_atom(testing::_))
        .Times(5)
        .WillRepeatedly(::testing::Invoke([&targets, &t_count](const string& handle) {
            Properties props;
            props["strength"] = 0.33 * (t_count + 1);
            vector<string> targets_ = {targets[t_count].first, targets[t_count].second};
            auto link = make_shared<Link>("Expression", targets_, props);
            t_count++;
            return link;
        }));
    auto ip = make_shared<EquivalenceProcessor>();
    shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>(1.0);
    query_answer->add_handle(A);
    query_answer->add_handle(B);
    auto links = ip->process_query(query_answer, vector<string>({"context"}));
    EXPECT_EQ(links.size(), 2);
    EXPECT_NEAR(links[0]->custom_attributes.get<double>("strength"), 0.9, 1e-2);
    EXPECT_NEAR(links[1]->custom_attributes.get<double>("strength"), 0.9, 1e-2);
}

TEST_F(LinkCreationAgentTest, TestMettaProcessorLinkCreation) {
    auto mp = make_shared<MettaTemplateProcessor>();
    string A = "A";
    string B = "B";
    string C = "C";
    string D = "D";
    shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>(1.0);
    query_answer->add_handle(A);
    query_answer->add_handle(B);
    query_answer->assignment.assign("V1", C);
    query_answer->assignment.assign("V2", D);
    auto mock_atomdb = dynamic_cast<AtomDBMock*>(AtomDBSingleton::get_instance().get());
    std::queue<string> target_queue;
    target_queue.push(C);
    target_queue.push(D);
    EXPECT_CALL(*mock_atomdb, get_atom(testing::_))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([&target_queue](const string& handle) {
            string name = target_queue.front();
            target_queue.pop();
            auto link = make_shared<Node>("Symbol", name);
            return link;
        }));
    auto links =
        mp->process_query(query_answer, vector<string>({"($V1 $V2 (Expression $V1 Test (Tttt $V2)))"}));
    EXPECT_EQ(links.size(), 1);
}

TEST_F(LinkCreationAgentTest, TestMettaProcessorLinkCreationInnerCheck) {
    auto mp = make_shared<MettaTemplateProcessor>();
    string A = "A";
    string B = "B";
    string C = "C";
    string D = "D";
    string Z = "Z";
    shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>(1.0);
    query_answer->add_handle(A);
    query_answer->add_handle(B);
    query_answer->assignment.assign("V1", C);
    query_answer->assignment.assign("V2", D);
    auto mock_atomdb = dynamic_cast<AtomDBMock*>(AtomDBSingleton::get_instance().get());
    std::queue<string> target_queue;
    target_queue.push(C);
    target_queue.push(D);
    EXPECT_CALL(*mock_atomdb, get_atom(testing::_))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([&target_queue](const string& handle) {
            string name = target_queue.front();
            target_queue.pop();
            auto link = make_shared<Node>("Symbol", name);
            return link;
        }));

    vector<string> expected_nodes = {"Tttt", "Test", C, Z, D};
    int nodes_count = 0;
    EXPECT_CALL(*mock_atomdb, add_node(testing::_, testing::_))
        .Times(expected_nodes.size())
        .WillRepeatedly(::testing::Invoke(
            [&expected_nodes, &nodes_count](const atoms::Node* node, bool throw_if_exists) {
                EXPECT_EQ(node->type, "Symbol");
                EXPECT_EQ(node->name, expected_nodes[nodes_count].c_str());
                nodes_count++;
                return "";
            }));
    string C_HASH = make_shared<Node>("Symbol", C)->handle();
    string Z_HASH = make_shared<Node>("Symbol", Z)->handle();
    string D_HASH = make_shared<Node>("Symbol", D)->handle();
    string TEST_HASH = make_shared<Node>("Symbol", "Test")->handle();
    string Tttt_HASH = make_shared<Node>("Symbol", "Tttt")->handle();
    string V1_Z_HASH =
        make_shared<Link>("Expression", vector<string>({C_HASH, Z_HASH}), Properties())->handle();
    string INNER_HASH =
        make_shared<Link>("Expression", vector<string>({Tttt_HASH, V1_Z_HASH, D_HASH}), Properties())
            ->handle();
    vector<vector<string>> expected_links = {
        {C_HASH, Z_HASH},
        {Tttt_HASH, V1_Z_HASH, D_HASH},
        {C_HASH, TEST_HASH, INNER_HASH},
    };
    int links_count = 0;
    EXPECT_CALL(*mock_atomdb, add_link(testing::_, testing::_))
        .Times(expected_links.size())
        .WillRepeatedly(::testing::Invoke(
            [&links_count, &expected_links](const atoms::Link* link, bool throw_if_exists) {
                EXPECT_EQ(link->type, "Expression");
                EXPECT_EQ(link->targets.size(), expected_links[links_count].size());
                for (size_t i = 0; i < expected_links[links_count].size(); i++) {
                    EXPECT_EQ(link->targets[i], expected_links[links_count][i]);
                }
                links_count++;
                return "";
            }));
    string metta_expression_str = "($V1 $V2 ($V1 Test (Tttt ($V1 Z) $V2)))";
    auto links = mp->process_query(query_answer, vector<string>({metta_expression_str}));
    EXPECT_EQ(links.size(), 1);
}

TEST_F(LinkCreationAgentTest, TestMettaProcessorLinkCreationSimpleMeta) {
    auto mp = make_shared<MettaTemplateProcessor>();
    string A = "A";
    string B = "B";
    string C = "C";
    string D = "D";
    string Z = "Z";
    shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>(1.0);
    query_answer->add_handle(A);
    query_answer->add_handle(B);
    query_answer->assignment.assign("V1", C);
    query_answer->assignment.assign("V2", D);
    auto mock_atomdb = dynamic_cast<AtomDBMock*>(AtomDBSingleton::get_instance().get());
    std::queue<string> target_queue;
    target_queue.push(C);
    target_queue.push(D);
    EXPECT_CALL(*mock_atomdb, get_atom(testing::_))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([&target_queue](const string& handle) {
            string name = target_queue.front();
            target_queue.pop();
            auto link = make_shared<Node>("Symbol", name);
            return link;
        }));

    vector<string> expected_nodes = {A, C, D};
    int nodes_count = 0;
    EXPECT_CALL(*mock_atomdb, add_node(testing::_, testing::_))
        .Times(expected_nodes.size())
        .WillRepeatedly(::testing::Invoke(
            [&expected_nodes, &nodes_count](const atoms::Node* node, bool throw_if_exists) {
                EXPECT_EQ(node->type, "Symbol");
                EXPECT_EQ(node->name, expected_nodes[nodes_count].c_str());
                nodes_count++;
                return "";
            }));
    // This should never be called
    EXPECT_CALL(*mock_atomdb, add_link(testing::_, testing::_)).Times(0);
    auto links = mp->process_query(query_answer, vector<string>({"(A $V1 $V2)"}));
    EXPECT_EQ(links.size(), 1);
}
