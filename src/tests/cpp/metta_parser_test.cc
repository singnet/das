#include "MettaTokens.h"
#include "MettaLexer.h"
#include "Utils.h"
#include "test_utils.h"
#include "gtest/gtest.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace metta;

static void check_tokens(
    const string& tag, 
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
            LOG_INFO(token_name[expected_tokens[i].first] + "<" + expected_tokens[i].second + "> " + token_name[(*tokens)[i]->value] + " <" + (*tokens)[i]->text + ">");
            EXPECT_EQ(expected_tokens[i].first, (*tokens)[i]->value);
            EXPECT_EQ(expected_tokens[i].second, (*tokens)[i]->text);
        }
    }
}

static void check_string(
    const string& tag, 
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
    check_string("04", "1.0", {make_pair(MettaTokens::FLOAT_LITERAL, "1.0")});
    check_string("05", "-1.0", {make_pair(MettaTokens::FLOAT_LITERAL, "-1.0")});
    check_string("06", "$a", {make_pair(MettaTokens::VARIABLE, "a")});
    check_string("07", "($a 1)", {
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::VARIABLE,          "a"),
        make_pair(MettaTokens::INTEGER_LITERAL,   "1"),
        make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
    });
    check_string("08", "(($v1 n1) $v2 n2)", {
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::VARIABLE,          "v1"),
        make_pair(MettaTokens::SYMBOL,            "n1"),
        make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
        make_pair(MettaTokens::VARIABLE,          "v2"),
        make_pair(MettaTokens::SYMBOL,            "n2"),
        make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
    });
    check_string("09", "($a \"blah\\\"bleh\\\"blih\\\"\")", {
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::VARIABLE,          "a"),
        make_pair(MettaTokens::STRING_LITERAL,   "\"blah\\\"bleh\\\"blih\\\"\""),
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
    check_tokens("multiple_strings", &tokens, {
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::SYMBOL,            "s1"),
        make_pair(MettaTokens::SYMBOL,            "s11"),
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::FLOAT_LITERAL,     "-4.0"),
        make_pair(MettaTokens::VARIABLE,          "v1"),
        make_pair(MettaTokens::VARIABLE,          "v2"),
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
    check_tokens("file_input", &tokens, {
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::SYMBOL,            "s1"),
        make_pair(MettaTokens::SYMBOL,            "s11"),
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::FLOAT_LITERAL,     "-4.0"),
        make_pair(MettaTokens::VARIABLE,          "v1"),
        make_pair(MettaTokens::VARIABLE,          "v2"),
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
    check_tokens("file_input", &tokens, {
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::SYMBOL,            "s1"),
        make_pair(MettaTokens::SYMBOL,            "s11"),
        make_pair(MettaTokens::OPEN_PARENTHESIS,  "("),
        make_pair(MettaTokens::FLOAT_LITERAL,     "-4.0"),
        make_pair(MettaTokens::VARIABLE,          "v1"),
        make_pair(MettaTokens::VARIABLE,          "v2"),
        make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
        make_pair(MettaTokens::CLOSE_PARENTHESIS, ")"),
    });
}
