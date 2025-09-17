#include "Link.h"
#include "MettaLexer.h"
#include "MettaMapping.h"
#include "MettaParser.h"
#include "MettaParserActions.h"
#include "MettaTokens.h"
#include "Node.h"
#include "Utils.h"
#include "gtest/gtest.h"
#include "test_utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atoms;
using namespace metta;

class TestActions : public ParserActions {
   public:
    vector<string> actions;
    TestActions() {}
    ~TestActions() {}
    void symbol(const string& name) override {
        ParserActions::symbol(name);
        actions.push_back("SYMBOL " + name);
    }
    void variable(const string& value) override {
        ParserActions::variable(value);
        actions.push_back("VARIABLE " + value);
    }
    void literal(const string& value) override {
        ParserActions::literal(value);
        actions.push_back("STRING_LITERAL " + value);
    }
    void literal(int value) override {
        ParserActions::literal(value);
        actions.push_back("INTEGER_LITERAL " + std::to_string(value));
    }
    void literal(float value) override {
        ParserActions::literal(value);
        actions.push_back("FLOAT_LITERAL " + std::to_string(value));
    }
    void expression_begin() override {
        ParserActions::expression_begin();
        actions.push_back("BEGIN_EXPRESSION");
    }
    void expression_end(bool toplevel, const string& metta_expression) override {
        ParserActions::expression_end(toplevel, metta_expression);
        actions.push_back((toplevel ? "TOPLEVEL_" : "") + string("EXPRESSION ") + metta_expression);
    }
};

static void check_tokens(const string& tag,
                         vector<unique_ptr<Token>>* tokens,
                         const vector<pair<unsigned char, string>>& expected_tokens) {
    LOG_INFO("--------------------------------------------------------------------------------");
    LOG_INFO("Test case " + tag);

    static map<unsigned char, string> token_name;
    token_name[1] = "OPEN_PARENTHESIS";
    token_name[2] = "CLOSE_PARENTHESIS";
    token_name[64] = "SYMBOL";
    token_name[129] = "STRING_LITERAL";
    token_name[130] = "INTEGER_LITERAL";
    token_name[131] = "FLOAT_LITERAL";
    token_name[192] = "VARIABLE";

    EXPECT_EQ(expected_tokens.size(), tokens->size());
    if (expected_tokens.size() == tokens->size()) {
        for (unsigned int i = 0; i < tokens->size(); i++) {
            LOG_INFO(token_name[expected_tokens[i].first] + "<" + expected_tokens[i].second + "> " +
                     token_name[(*tokens)[i]->type] + " <" + (*tokens)[i]->text + ">");
            EXPECT_EQ(expected_tokens[i].first, (*tokens)[i]->type);
            EXPECT_EQ(expected_tokens[i].second, (*tokens)[i]->text);
        }
    }
}

static void check_string(const string& tag,
                         const string& metta_string,
                         const vector<pair<unsigned char, string>>& expected_tokens) {
    MettaLexer lexer(metta_string);
    vector<unique_ptr<Token>> tokens;
    unique_ptr<Token> token;
    while ((token = lexer.next()) != nullptr) {
        tokens.push_back(move(token));
    }

    check_tokens(tag, &tokens, expected_tokens);
}

static void check_parse(const string& tag,
                        const string& metta_str,
                        bool syntax_ok,
                        const vector<string>& expected) {
    LOG_INFO("--------------------------------------------------------------------------------");
    LOG_INFO("Test case " + tag);

    shared_ptr<TestActions> broker = make_shared<TestActions>();
    MettaParser parser(metta_str, broker);
    if (syntax_ok) {
        EXPECT_FALSE(parser.parse(false));
    } else {
        EXPECT_TRUE(parser.parse(false));
    }
    if (expected.size() > 0) {
        EXPECT_EQ(expected.size(), broker->actions.size());
        for (unsigned int i = 0; i < expected.size(); i++) {
            LOG_INFO(expected[i] + " - " + broker->actions[i]);
            EXPECT_EQ(expected[i], broker->actions[i]);
        }
    }
}

TEST(MettaTokens, basics) {
    EXPECT_TRUE(MettaTokens::is_syntax_marker(MettaTokens::OPEN_PARENTHESIS));
    EXPECT_FALSE(MettaTokens::is_symbol(MettaTokens::OPEN_PARENTHESIS));
    EXPECT_FALSE(MettaTokens::is_literal(MettaTokens::OPEN_PARENTHESIS));
    EXPECT_FALSE(MettaTokens::is_variable(MettaTokens::OPEN_PARENTHESIS));

    EXPECT_TRUE(MettaTokens::is_syntax_marker(MettaTokens::CLOSE_PARENTHESIS));
    EXPECT_FALSE(MettaTokens::is_symbol(MettaTokens::CLOSE_PARENTHESIS));
    EXPECT_FALSE(MettaTokens::is_literal(MettaTokens::CLOSE_PARENTHESIS));
    EXPECT_FALSE(MettaTokens::is_variable(MettaTokens::CLOSE_PARENTHESIS));

    EXPECT_FALSE(MettaTokens::is_syntax_marker(MettaTokens::SYMBOL));
    EXPECT_TRUE(MettaTokens::is_symbol(MettaTokens::SYMBOL));
    EXPECT_FALSE(MettaTokens::is_literal(MettaTokens::SYMBOL));
    EXPECT_FALSE(MettaTokens::is_variable(MettaTokens::SYMBOL));

    EXPECT_FALSE(MettaTokens::is_syntax_marker(MettaTokens::STRING_LITERAL));
    EXPECT_FALSE(MettaTokens::is_symbol(MettaTokens::STRING_LITERAL));
    EXPECT_TRUE(MettaTokens::is_literal(MettaTokens::STRING_LITERAL));
    EXPECT_FALSE(MettaTokens::is_variable(MettaTokens::STRING_LITERAL));

    EXPECT_FALSE(MettaTokens::is_syntax_marker(MettaTokens::INTEGER_LITERAL));
    EXPECT_FALSE(MettaTokens::is_symbol(MettaTokens::INTEGER_LITERAL));
    EXPECT_TRUE(MettaTokens::is_literal(MettaTokens::INTEGER_LITERAL));
    EXPECT_FALSE(MettaTokens::is_variable(MettaTokens::INTEGER_LITERAL));

    EXPECT_FALSE(MettaTokens::is_syntax_marker(MettaTokens::FLOAT_LITERAL));
    EXPECT_FALSE(MettaTokens::is_symbol(MettaTokens::FLOAT_LITERAL));
    EXPECT_TRUE(MettaTokens::is_literal(MettaTokens::FLOAT_LITERAL));
    EXPECT_FALSE(MettaTokens::is_variable(MettaTokens::FLOAT_LITERAL));

    EXPECT_FALSE(MettaTokens::is_syntax_marker(MettaTokens::VARIABLE));
    EXPECT_FALSE(MettaTokens::is_symbol(MettaTokens::VARIABLE));
    EXPECT_FALSE(MettaTokens::is_literal(MettaTokens::VARIABLE));
    EXPECT_TRUE(MettaTokens::is_variable(MettaTokens::VARIABLE));
}

TEST(MettaLexer, basics) {
    MettaLexer lexer1("blah");
    EXPECT_THROW(lexer1.attach_string("blah"), runtime_error);
    EXPECT_THROW(lexer1.attach_file("blah"), runtime_error);
    MettaLexer lexer2;
    lexer2.attach_file("blah");
    EXPECT_THROW(lexer2.attach_string("blah"), runtime_error);
}

TEST(MettaLexer, string_input) {
    check_string("01", "", {});
    check_string("02", "a", {make_pair(MettaTokens::SYMBOL, "a")});
    check_string("03", "\"a\"", {make_pair(MettaTokens::STRING_LITERAL, "\"a\"")});
    check_string("04", "1", {make_pair(MettaTokens::INTEGER_LITERAL, "1")});
    check_string("05", "1.0", {make_pair(MettaTokens::FLOAT_LITERAL, "1.0")});
    check_string("06", "-1.0", {make_pair(MettaTokens::FLOAT_LITERAL, "-1.0")});
    check_string("07", "$a", {make_pair(MettaTokens::VARIABLE, "a")});
    check_string("08",
                 "($a 1)",
                 {
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::VARIABLE, "a"),
                     make_pair(MettaTokens::INTEGER_LITERAL, "1"),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                 });
    check_string("09",
                 "(($v1 n1) $v2 n2)",
                 {
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::VARIABLE, "v1"),
                     make_pair(MettaTokens::SYMBOL, "n1"),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                     make_pair(MettaTokens::VARIABLE, "v2"),
                     make_pair(MettaTokens::SYMBOL, "n2"),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                 });
    check_string("10",
                 "($a \"blah\\\"bleh\\\"blih\\\"\")",
                 {
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::VARIABLE, "a"),
                     make_pair(MettaTokens::STRING_LITERAL, "\"blah\\\"bleh\\\"blih\\\"\""),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                 });
}

TEST(MettaLexer, multiple_strings) {
    // Full MeTTa expression "(s1 s11 (-4.0 $v1 $v2))";
    string s1 = "(s1 s11 (-";
    string s2 = "4.0 $v1 $v";
    string s3 = "2";
    string s4 = ")";
    string s5 = ")";
    MettaLexer lexer(10);
    lexer.attach_string(s1);
    lexer.attach_string(s2);
    lexer.attach_string(s3);
    lexer.attach_string(s4);
    lexer.attach_string(s5);
    vector<unique_ptr<Token>> tokens;
    unique_ptr<Token> token;
    while ((token = lexer.next()) != nullptr) {
        tokens.push_back(move(token));
    }
    check_tokens("multiple_strings",
                 &tokens,
                 {
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::SYMBOL, "s1"),
                     make_pair(MettaTokens::SYMBOL, "s11"),
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::FLOAT_LITERAL, "-4.0"),
                     make_pair(MettaTokens::VARIABLE, "v1"),
                     make_pair(MettaTokens::VARIABLE, "v2"),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                 });
}

TEST(MettaLexer, file_input) {
    // Full MeTTa expression "(s1 s11 (-4.0 $v1 $v2))";
    make_tmp_file("MettaLexer_file_input_f1", "(s1 s11 (-");
    make_tmp_file("MettaLexer_file_input_f2", "4.0 $v1 $v");
    make_tmp_file("MettaLexer_file_input_f3", "2");
    make_tmp_file("MettaLexer_file_input_f4", ")");
    make_tmp_file("MettaLexer_file_input_f5", ")");
    MettaLexer lexer(10);
    lexer.attach_file("/tmp/MettaLexer_file_input_f1");
    lexer.attach_file("/tmp/MettaLexer_file_input_f2");
    lexer.attach_file("/tmp/MettaLexer_file_input_f3");
    lexer.attach_file("/tmp/MettaLexer_file_input_f4");
    lexer.attach_file("/tmp/MettaLexer_file_input_f5");
    vector<unique_ptr<Token>> tokens;
    unique_ptr<Token> token;
    while ((token = lexer.next()) != nullptr) {
        tokens.push_back(move(token));
    }
    check_tokens("file_input",
                 &tokens,
                 {
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::SYMBOL, "s1"),
                     make_pair(MettaTokens::SYMBOL, "s11"),
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::FLOAT_LITERAL, "-4.0"),
                     make_pair(MettaTokens::VARIABLE, "v1"),
                     make_pair(MettaTokens::VARIABLE, "v2"),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                 });
}

TEST(MettaLexer, file_large) {
    make_tmp_file("MettaLexer_file_large", "(s1 s11 (-4.0 $v1 $v2))");
    MettaLexer lexer(10);
    lexer.attach_file("/tmp/MettaLexer_file_large");
    vector<unique_ptr<Token>> tokens;
    unique_ptr<Token> token;
    while ((token = lexer.next()) != nullptr) {
        tokens.push_back(move(token));
    }
    check_tokens("file_input",
                 &tokens,
                 {
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::SYMBOL, "s1"),
                     make_pair(MettaTokens::SYMBOL, "s11"),
                     make_pair(MettaTokens::OPEN_PARENTHESIS, "("),
                     make_pair(MettaTokens::FLOAT_LITERAL, "-4.0"),
                     make_pair(MettaTokens::VARIABLE, "v1"),
                     make_pair(MettaTokens::VARIABLE, "v2"),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                     make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
                 });
}

TEST(MettaParser, basics) {
    check_parse("01", "", true, {});
    check_parse("02", "a", true, {"SYMBOL a"});
    check_parse("03", "\"a\"", true, {"STRING_LITERAL \"a\""});
    check_parse("04", "1", true, {"INTEGER_LITERAL 1"});
    check_parse("05", "1.0", true, {"FLOAT_LITERAL " + std::to_string(1.0)});
    check_parse("06", "-1.0", true, {"FLOAT_LITERAL " + std::to_string(-1.0)});
    check_parse("07", "$a", true, {"VARIABLE a"});
    check_parse("08",
                "($a 1)",
                true,
                {
                    "BEGIN_EXPRESSION",
                    "VARIABLE a",
                    "INTEGER_LITERAL 1",
                    "TOPLEVEL_EXPRESSION ($a 1)",
                });
    check_parse("09",
                "(($v1 n1) $v2 n2)",
                true,
                {
                    "BEGIN_EXPRESSION",
                    "BEGIN_EXPRESSION",
                    "VARIABLE v1",
                    "SYMBOL n1",
                    "EXPRESSION ($v1 n1)",
                    "VARIABLE v2",
                    "SYMBOL n2",
                    "TOPLEVEL_EXPRESSION (($v1 n1) $v2 n2)",
                });
    check_parse("10",
                "($a \"blah\\\"bleh\\\"blih\\\"\")",
                true,
                {
                    "BEGIN_EXPRESSION",
                    "VARIABLE a",
                    "STRING_LITERAL \"blah\\\"bleh\\\"blih\\\"\"",
                    "TOPLEVEL_EXPRESSION ($a \"blah\\\"bleh\\\"blih\\\"\")",
                });
    // clang-format off
    check_parse(
        "11",
        "(($v1 n1) (2) $v2 (n8) ($v4) 1 n2 (n4 n5 (n6 (n7 $v3)))) (n10 n11)",
        true,
        {
            "BEGIN_EXPRESSION",
            "BEGIN_EXPRESSION",
            "VARIABLE v1",
            "SYMBOL n1",
            "EXPRESSION ($v1 n1)",
            "BEGIN_EXPRESSION",
            "INTEGER_LITERAL 2",
            "EXPRESSION (2)",
            "VARIABLE v2",
            "BEGIN_EXPRESSION",
            "SYMBOL n8",
            "EXPRESSION (n8)",
            "BEGIN_EXPRESSION",
            "VARIABLE v4",
            "EXPRESSION ($v4)",
            "INTEGER_LITERAL 1",
            "SYMBOL n2",
            "BEGIN_EXPRESSION",
            "SYMBOL n4",
            "SYMBOL n5",
            "BEGIN_EXPRESSION",
            "SYMBOL n6",
            "BEGIN_EXPRESSION",
            "SYMBOL n7",
            "VARIABLE v3",
            "EXPRESSION (n7 $v3)",
            "EXPRESSION (n6 (n7 $v3))",
            "EXPRESSION (n4 n5 (n6 (n7 $v3)))",
            "TOPLEVEL_EXPRESSION (($v1 n1) (2) $v2 (n8) ($v4) 1 n2 (n4 n5 (n6 (n7 $v3))))",
            "BEGIN_EXPRESSION",
            "SYMBOL n10",
            "SYMBOL n11",
            "TOPLEVEL_EXPRESSION (n10 n11)",
        });
    // clang-format on
}

TEST(MettaParser, Atoms_MettaParserActions) {
    shared_ptr<MettaParserActions> pa = make_shared<MettaParserActions>();
    MettaParser parser = MettaParser("(s1 s11 (-4.0 \"literal\" 7))", pa);
    EXPECT_FALSE(parser.parse(true));
    EXPECT_EQ(pa->handle_to_atom.size(), 7);

    pa = make_shared<MettaParserActions>();
    parser = MettaParser("(s1 s11 (-4.0 $v1 $v2) (8 9))", pa);
    EXPECT_FALSE(parser.parse(true));
    EXPECT_EQ(pa->handle_to_atom.size(), 10);

    pa = make_shared<MettaParserActions>();
    parser = MettaParser("(($v1 n1) (2) $v2 (n8) ($v4) 1 n2 (n4 n5 (n6 (n7 $v3)))) (n10 n11)", pa);
    EXPECT_FALSE(parser.parse(true));
    EXPECT_EQ(pa->handle_to_atom.size(), 24);

    pa = make_shared<MettaParserActions>();
    parser = MettaParser("(1) (2) (3)", pa);
    EXPECT_FALSE(parser.parse(true));
    EXPECT_EQ(pa->handle_to_atom.size(), 6);

    pa = make_shared<MettaParserActions>();
    parser = MettaParser("((1) (2)) (3)", pa);
    EXPECT_FALSE(parser.parse(true));
    EXPECT_EQ(pa->handle_to_atom.size(), 7);

    pa = make_shared<MettaParserActions>();
    parser = MettaParser("(and (a) (b))", pa);
    EXPECT_THROW(parser.parse(true), runtime_error);

    pa = make_shared<MettaParserActions>();
    parser = MettaParser("(or (a) (b))", pa);
    EXPECT_THROW(parser.parse(true), runtime_error);

    pa = make_shared<MettaParserActions>();
    parser = MettaParser("(and (or (a) (b)) (and (c) (d)))", pa);
    EXPECT_THROW(parser.parse(true), runtime_error);

    // Similarity (simple expression)
    pa = make_shared<MettaParserActions>();
    parser = MettaParser("(Similarity \"human\" \"monkey\")", pa);
    EXPECT_FALSE(parser.parse(true));
    EXPECT_EQ(pa->handle_to_atom.size(), 4);

    auto similarty_node = new Node(MettaMapping::SYMBOL_NODE_TYPE, "Similarity");
    auto human_node = new Node(MettaMapping::SYMBOL_NODE_TYPE, "\"human\"");
    auto monkey_node = new Node(MettaMapping::SYMBOL_NODE_TYPE, "\"monkey\"");

    auto similarity_link =
        new Link(MettaMapping::EXPRESSION_LINK_TYPE,
                 {similarty_node->handle(), human_node->handle(), monkey_node->handle()});
    auto invalid_similarity_link =
        new Link(MettaMapping::EXPRESSION_LINK_TYPE,
                 {similarty_node->handle(), monkey_node->handle(), human_node->handle()});

    // Check handle_to_atom
    EXPECT_EQ(pa->handle_to_atom[similarty_node->handle()]->handle(), similarty_node->handle());
    EXPECT_EQ(pa->handle_to_atom[human_node->handle()]->handle(), human_node->handle());
    EXPECT_EQ(pa->handle_to_atom[monkey_node->handle()]->handle(), monkey_node->handle());
    EXPECT_EQ(pa->handle_to_atom[similarity_link->handle()]->handle(), similarity_link->handle());
    EXPECT_EQ(pa->handle_to_atom[invalid_similarity_link->handle()], nullptr);
    // Check handle_to_metta_expression
    EXPECT_EQ(pa->handle_to_metta_expression[similarty_node->handle()], "Similarity");
    EXPECT_EQ(pa->handle_to_metta_expression[human_node->handle()], "\"human\"");
    EXPECT_EQ(pa->handle_to_metta_expression[monkey_node->handle()], "\"monkey\"");
    EXPECT_EQ(pa->handle_to_metta_expression[similarity_link->handle()],
              "(Similarity \"human\" \"monkey\")");
    EXPECT_EQ(pa->handle_to_metta_expression[invalid_similarity_link->handle()], "");

    // EVALUATION (nested expression)
    pa = make_shared<MettaParserActions>();
    parser = MettaParser("(EVALUATION (PREDICATE \"is_animal\") (CONCEPT \"human\"))", pa);
    EXPECT_FALSE(parser.parse(true));
    EXPECT_EQ(pa->handle_to_atom.size(), 8);

    auto evaluation_node = new Node(MettaMapping::SYMBOL_NODE_TYPE, "EVALUATION");
    auto predicate_node = new Node(MettaMapping::SYMBOL_NODE_TYPE, "PREDICATE");
    auto is_animal_node = new Node(MettaMapping::SYMBOL_NODE_TYPE, "\"is_animal\"");
    auto concept_node = new Node(MettaMapping::SYMBOL_NODE_TYPE, "CONCEPT");

    auto predicate_link = new Link(MettaMapping::EXPRESSION_LINK_TYPE,
                                   {predicate_node->handle(), is_animal_node->handle()});
    auto concept_link =
        new Link(MettaMapping::EXPRESSION_LINK_TYPE, {concept_node->handle(), human_node->handle()});

    auto evaluation_link =
        new Link(MettaMapping::EXPRESSION_LINK_TYPE,
                 {evaluation_node->handle(), predicate_link->handle(), concept_link->handle()});

    // Check handle_to_atom
    EXPECT_EQ(pa->handle_to_atom[evaluation_node->handle()]->handle(), evaluation_node->handle());
    EXPECT_EQ(pa->handle_to_atom[predicate_node->handle()]->handle(), predicate_node->handle());
    EXPECT_EQ(pa->handle_to_atom[is_animal_node->handle()]->handle(), is_animal_node->handle());
    EXPECT_EQ(pa->handle_to_atom[concept_node->handle()]->handle(), concept_node->handle());
    EXPECT_EQ(pa->handle_to_atom[human_node->handle()]->handle(), human_node->handle());
    EXPECT_EQ(pa->handle_to_atom[evaluation_link->handle()]->handle(), evaluation_link->handle());
    EXPECT_EQ(pa->handle_to_atom[predicate_link->handle()]->handle(), predicate_link->handle());
    EXPECT_EQ(pa->handle_to_atom[concept_link->handle()]->handle(), concept_link->handle());
    // Check handle_to_metta_expression
    EXPECT_EQ(pa->handle_to_metta_expression[evaluation_node->handle()], "EVALUATION");
    EXPECT_EQ(pa->handle_to_metta_expression[predicate_node->handle()], "PREDICATE");
    EXPECT_EQ(pa->handle_to_metta_expression[concept_node->handle()], "CONCEPT");
    EXPECT_EQ(pa->handle_to_metta_expression[is_animal_node->handle()], "\"is_animal\"");
    EXPECT_EQ(pa->handle_to_metta_expression[human_node->handle()], "\"human\"");
    EXPECT_EQ(pa->handle_to_metta_expression[evaluation_link->handle()],
              "(EVALUATION (PREDICATE \"is_animal\") (CONCEPT \"human\"))");
    EXPECT_EQ(pa->handle_to_metta_expression[predicate_link->handle()], "(PREDICATE \"is_animal\")");
    EXPECT_EQ(pa->handle_to_metta_expression[concept_link->handle()], "(CONCEPT \"human\")");
}
