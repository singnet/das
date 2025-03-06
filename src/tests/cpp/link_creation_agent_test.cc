#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <thread>

#include "link_creation_agent.h"
#include "link_create_template.h"

using namespace std;
using namespace link_creation_agent;

class LinkCreationAgentTest : public ::testing::Test {
   protected:
    LinkCreationAgent* agent;

    void SetUp() override {
        // Create a temporary config file for testing
        ofstream config_file("test_config.cfg");
        config_file << "requests_interval_seconds=1\n";
        config_file << "link_creation_agent_thread_count=1\n";
        config_file << "query_agent_client_id=localhost:7001\n";
        config_file << "query_agent_server_id=localhost:7002\n";
        config_file << "link_creation_agent_server_id=localhost:7003\n";
        config_file << "das_agent_client_id=localhost:7004\n";
        config_file << "das_agent_server_id=localhost:7005\n";
        config_file << "requests_buffer_file=test_buffer.bin\n";
        config_file.close();
    }

    void TearDown() override {
        remove("test_config.cfg");
        remove("test_buffer.bin");
    }
};

TEST_F(LinkCreationAgentTest, TestRequest) {
    // Simulate a request
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
}

TEST_F(LinkCreationAgentTest, TestConfig) {
    agent = new LinkCreationAgent("test_config.cfg");
    delete agent;
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
    EXPECT_EQ(get<Node>(lct5.get_targets()[0]).type, "NodeType1");
    EXPECT_EQ(get<Node>(lct5.get_targets()[0]).value, "Value1");
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
    EXPECT_EQ(get<Node>(lct6.get_targets()[1]).type, "NodeTypeA");
    EXPECT_EQ(get<Node>(lct6.get_targets()[1]).value, "ValueA");
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
    EXPECT_EQ(get<Node>(lct7.get_targets()[0]).type, "Type1");
    EXPECT_EQ(get<Node>(lct7.get_targets()[0]).value, "Val1");
    EXPECT_EQ(get<Variable>(lct7.get_targets()[1]).name, "Var1");
    EXPECT_EQ(get<Node>(lct7.get_targets()[2]).type, "Type2");
    EXPECT_EQ(get<Node>(lct7.get_targets()[2]).value, "Val2");
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
    EXPECT_EQ(get<Node>(lct8.get_targets()[1]).type, "SimpleNode");
    EXPECT_EQ(get<Node>(lct8.get_targets()[1]).value, "SimpleValue");
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


TEST(Link, TestLink) {
    vector<string> link_template = split("LINK_CREATE Similarity 2 0 VARIABLE V1 VARIABLE V2", ' ');
    HandlesAnswer* query_answer = new HandlesAnswer();
    query_answer->assignment.assign("V1", "Value1");
    query_answer->assignment.assign("V2", "Value2");

    Link link(query_answer, link_template);
    EXPECT_EQ(link.get_type(), "Similarity");
    EXPECT_EQ(link.get_targets().size(), 2);
    EXPECT_EQ(get<string>(link.get_targets()[0]), "Value1");
    EXPECT_EQ(get<string>(link.get_targets()[1]), "Value2");
}
