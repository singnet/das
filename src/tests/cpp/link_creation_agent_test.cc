#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <thread>

#include "AtomDBSingleton.h"
#include "LinkCreateTemplate.h"
#include "LinkCreationAgent.h"
#include "LinkCreationRequestProcessor.h"
#include "TemplateProcessor.h"
#include "Utils.h"

using namespace std;
using namespace link_creation_agent;
using namespace commons;
using namespace service_bus;
using namespace atomdb;

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
        this->server_id = "localhost:7003";
    }

    void TearDown() override { remove("test_buffer.bin"); }
};

TEST_F(LinkCreationAgentTest, TestRequest) {
    vector<string> request = {
        "query1", "LINK_CREATE", "test", "1", "0", "VARIABLE", "V1", "10", "5", "test_context", "true"};
    shared_ptr<LinkCreationAgentRequest> lca_request = LinkCreationAgent::create_request(request);
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

    request = {"query2",
               "LINK_CREATE",
               "test2",
               "2",
               "1",
               "NODE",
               "Symbol",
               "A",
               "VARIABLE",
               "V1",
               "10",
               "-1",
               "test_context",
               "false",
               "1"};
    lca_request = LinkCreationAgent::create_request(request);
    EXPECT_EQ(lca_request->query, vector<string>({"query2"}));
    EXPECT_EQ(
        lca_request->link_template,
        vector<string>({"LINK_CREATE", "test2", "2", "1", "NODE", "Symbol", "A", "VARIABLE", "V1"}));
    EXPECT_EQ(lca_request->max_results, 10);
    EXPECT_EQ(lca_request->repeat, -1);
    EXPECT_EQ(lca_request->context, "test_context");
    EXPECT_EQ(lca_request->update_attention_broker, false);
    EXPECT_EQ(lca_request->infinite, true);
    EXPECT_EQ(lca_request->id, "c4ca4238a0b923820dcc509a6f75849b");
}

TEST_F(LinkCreationAgentTest, TestConfig) {
    ServiceBusSingleton::init(this->server_id);
    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(
        make_shared<LinkCreationRequestProcessor>(this->request_interval,
                                                  this->thread_count,
                                                  this->default_timeout,
                                                  this->buffer_file_path,
                                                  this->metta_file_path,
                                                  this->save_links_to_metta_file,
                                                  this->save_links_to_db));
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
    /**  #NOTE Different from the original string test, the to_string() method is not returning the same
       order as the input string. to_string is placing the custom fields after the targets
    */
    string link_template_str =
        "LINK_CREATE Similarity 3 1 VARIABLE V1 LINK_CREATE Test 3 0 NODE Symbol A VARIABLE V2 "
        "LINK_CREATE Test2 1 1 NODE Symbol C CUSTOM_FIELD inter 1 inter_name inter_value NODE Symbol B "
        "CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9";
    auto link_template = split(link_template_str, ' ');
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
    EXPECT_EQ(get<Variable>(lct.get_targets()[0]).name, "V1");
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
    EXPECT_EQ(get<LCANode>(lct5.get_targets()[0]).type, "NodeType1");
    EXPECT_EQ(get<LCANode>(lct5.get_targets()[0]).value, "Value1");
    EXPECT_EQ(get<Variable>(lct5.get_targets()[1]).name, "Var1");
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
    EXPECT_EQ(get<Variable>(lct6.get_targets()[0]).name, "VarA");
    EXPECT_EQ(get<LCANode>(lct6.get_targets()[1]).type, "NodeTypeA");
    EXPECT_EQ(get<LCANode>(lct6.get_targets()[1]).value, "ValueA");
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
    EXPECT_EQ(get<LCANode>(lct7.get_targets()[0]).type, "Type1");
    EXPECT_EQ(get<LCANode>(lct7.get_targets()[0]).value, "Val1");
    EXPECT_EQ(get<Variable>(lct7.get_targets()[1]).name, "Var1");
    EXPECT_EQ(get<LCANode>(lct7.get_targets()[2]).type, "Type2");
    EXPECT_EQ(get<LCANode>(lct7.get_targets()[2]).value, "Val2");
    EXPECT_EQ(get<Variable>(lct7.get_targets()[3]).name, "Var2");
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
    EXPECT_EQ(get<Variable>(lct8.get_targets()[0]).name, "SimpleVar");
    EXPECT_EQ(get<LCANode>(lct8.get_targets()[1]).type, "SimpleNode");
    EXPECT_EQ(get<LCANode>(lct8.get_targets()[1]).value, "SimpleValue");
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
    EXPECT_THROW(LinkCreateTemplate lct(link_template), invalid_argument);
}

TEST(LinkCreateTemplate, TestInvalidCustomField) {
    string link_template_str =
        "LINK_CREATE Similarity 2 1 VARIABLE V1 NODE Symbol A CUSTOM_FIELD field1";
    auto link_template = split(link_template_str, ' ');
    EXPECT_THROW(LinkCreateTemplate lct(link_template), invalid_argument);
}

TEST(LinkCreateTemplate, TestInvalidVariable) {
    string link_template_str = "LINK_CREATE Similarity 2 1 VARIABLE NODE Symbol A";
    auto link_template = split(link_template_str, ' ');
    EXPECT_THROW(LinkCreateTemplate lct(link_template), invalid_argument);
}

TEST(LinkCreateTemplate, TestInvalidNode) {
    string link_template_str = "LINK_CREATE Similarity 2 1 VARIABLE V1 NODE Symbol";
    auto link_template = split(link_template_str, ' ');
    EXPECT_THROW(LinkCreateTemplate lct(link_template), invalid_argument);
}

TEST(Link, TestLinkTemplateProcessor) {
    vector<string> link_template = split("LINK_CREATE Similarity 2 0 VARIABLE V1 VARIABLE V2", ' ');
    shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>();
    query_answer->assignment.assign("V1", "Value1");
    query_answer->assignment.assign("V2", "Value2");

    LinkTemplateProcessor ltp;
    auto links = ltp.process(query_answer, link_template);
    EXPECT_EQ(Utils::join(links[0], ' '), "LINK Similarity 2 0 HANDLE Value1 HANDLE Value2");
    // EXPECT_EQ(link.to_metta_string(), "(Value1 Value2)");
    link_template.clear();

    query_answer = make_shared<QueryAnswer>();
    link_template = split("LINK_CREATE Test 3 0 NODE Symbol A VARIABLE V1 NODE Symbol B", ' ');
    query_answer->assignment.assign("V1", "Value1");
    links = ltp.process(query_answer, link_template);
    EXPECT_EQ(Utils::join(links[0], ' '), "LINK Test 3 0 NODE Symbol A HANDLE Value1 NODE Symbol B");
    // EXPECT_EQ(link.to_metta_string(), "(A Value1 B)");

    link_template.clear();

    query_answer = make_shared<QueryAnswer>();
    link_template = split(
        "LINK_CREATE Test2 2 1 NODE Symbol C NODE Symbol B "
        "CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9",
        ' ');
    links = ltp.process(query_answer, link_template);
    EXPECT_EQ(
        Utils::join(links[0], ' '),
        "LINK Test2 2 1 NODE Symbol C NODE Symbol B CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 "
        "count 10 avg 0.9 confidence 0.9");

    link_template.clear();

    query_answer = make_shared<QueryAnswer>();
    link_template = split(
        "LINK_CREATE Test3 2 1 VARIABLE V1 VARIABLE V2 "
        "CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9",
        ' ');
    query_answer->assignment.assign("V1", "Value1");
    query_answer->assignment.assign("V2", "Value2");
    links = ltp.process(query_answer, link_template);
    EXPECT_EQ(
        Utils::join(links[0], ' '),
        "LINK Test3 2 1 HANDLE Value1 HANDLE Value2 CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 "
        "count 10 avg 0.9 confidence 0.9");
    link_template.clear();

    // Test the Link class
    vector<string> tokens = Utils::split(
        "LINK Test3 2 1 HANDLE Value1 HANDLE Value2 CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 "
        "count 10 avg 0.9 confidence 0.9",
        ' ');
    LCALink ll;
    LCALink l2 = ll.untokenize(tokens);
    EXPECT_EQ(l2.get_type(), "Test3");
    EXPECT_EQ(l2.get_targets().size(), 2);
    EXPECT_EQ(get<string>(l2.get_targets()[0]), "Value1");
    EXPECT_EQ(get<string>(l2.get_targets()[1]), "Value2");
    EXPECT_EQ(l2.get_custom_fields().size(), 1);
    CustomField truth_value = l2.get_custom_fields()[0];
    EXPECT_EQ(truth_value.get_name(), "truth_value");
    EXPECT_EQ(truth_value.get_values().size(), 2);
    shared_ptr<CustomField> mean = get<shared_ptr<CustomField>>(get<1>(truth_value.get_values()[0]));
    EXPECT_EQ(mean->get_values().size(), 2);
    EXPECT_EQ(get<0>(mean->get_values()[0]), "count");
    EXPECT_EQ(get<string>(get<1>(mean->get_values()[0])), "10");
    EXPECT_EQ(get<0>(mean->get_values()[1]), "avg");
    EXPECT_EQ(get<string>(get<1>(mean->get_values()[1])), "0.9");
    EXPECT_EQ(get<0>(l2.get_custom_fields()[0].get_values()[1]), "confidence");
    EXPECT_EQ(get<string>(get<1>(l2.get_custom_fields()[0].get_values()[1])), "0.9");
    EXPECT_EQ(l2.to_metta_string(), "(Value1 [[count 10 avg 0.9] confidence 0.9] Value2)");
    tokens.clear();

    tokens = Utils::split(
        "LINK Test3 3 1 NODE Symbol EQUIVALENCE HANDLE Value1 HANDLE Value2 CUSTOM_FIELD truth_value 2 "
        "strength 0.9 confidence 1",
        ' ');
    l2 = ll.untokenize(tokens);
    EXPECT_EQ(l2.get_type(), "Test3");
    EXPECT_EQ(l2.get_targets().size(), 3);
    // EXPECT_EQ(get<LCANode>(l2.get_targets()[0]), "EQUIVALENCE");
    EXPECT_EQ(get<string>(l2.get_targets()[1]), "Value1");
    EXPECT_EQ(get<string>(l2.get_targets()[2]), "Value2");
    EXPECT_EQ(l2.get_custom_fields().size(), 1);
    truth_value = l2.get_custom_fields()[0];
    EXPECT_EQ(truth_value.get_name(), "truth_value");
    EXPECT_EQ(truth_value.get_values().size(), 2);
    EXPECT_EQ(get<0>(l2.get_custom_fields()[0].get_values()[0]), "strength");
    EXPECT_EQ(get<string>(get<1>(l2.get_custom_fields()[0].get_values()[0])), "0.9");
    EXPECT_EQ(get<0>(l2.get_custom_fields()[0].get_values()[1]), "confidence");
    EXPECT_EQ(get<string>(get<1>(l2.get_custom_fields()[0].get_values()[1])), "1");
    EXPECT_EQ(l2.to_metta_string(), "(EQUIVALENCE [strength 0.9 confidence 1] Value1 Value2)");
}
