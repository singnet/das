#include "Keychain.h"

#include <gtest/gtest.h>

#include <map>
#include <string>

#include "Utils.h"

using namespace atomdb;
using namespace std;
using namespace commons;

TEST(KeychainTest, GetPublicKeyReturnsEmpty) {
    Keychain empty_keychain1(map<string, string>{});
    EXPECT_EQ(empty_keychain1.get_public_key("blah"), "");
    EXPECT_EQ(empty_keychain1.get_public_key(""), "");

    Keychain empty_keychain2;
    empty_keychain2.untokenize(vector<string>{});
    EXPECT_EQ(empty_keychain2.get_public_key("blah"), "");
    EXPECT_EQ(empty_keychain2.get_public_key(""), "");

    Keychain keychain1(map<string, string>{{"uid1", "key1"}});
    EXPECT_EQ(keychain1.get_public_key("uid2"), "");
    EXPECT_EQ(keychain1.get_public_key("uid"), "");
    EXPECT_EQ(keychain1.get_public_key(""), "");

    Keychain keychain2;
    keychain2.untokenize(vector<string>{"uid1", "key1"});
    EXPECT_EQ(keychain2.get_public_key("uid2"), "");
    EXPECT_EQ(keychain2.get_public_key("uid"), "");
    EXPECT_EQ(keychain2.get_public_key(""), "");

    Keychain empty_stored_key1(map<string, string>{{"uid", ""}});
    EXPECT_EQ(empty_stored_key1.get_public_key("uid"), "");

    Keychain empty_stored_key2;
    empty_stored_key2.untokenize(vector<string>{"uid", ""});
    EXPECT_EQ(empty_stored_key2.get_public_key("uid"), "");
}

TEST(KeychainTest, GetPublicKeyReturnsStoredKey) {
    Keychain keychain1(map<string, string>{{"uid1", "key1"}, {"uid2", "key2"}});
    EXPECT_EQ(keychain1.get_public_key("uid1"), "key1");
    EXPECT_EQ(keychain1.get_public_key("uid2"), "key2");

    Keychain keychain2;
    keychain2.untokenize(vector<string>{"uid1", "key1", "uid2", "key2"});
    EXPECT_EQ(keychain2.get_public_key("uid1"), "key1");
    EXPECT_EQ(keychain2.get_public_key("uid2"), "key2");
}

TEST(KeychainTest, MalformedInitializationVector) {
    Keychain keychain;
    EXPECT_THROW(keychain.untokenize(vector<string>{"uid1"}), runtime_error);
    EXPECT_THROW(keychain.untokenize(vector<string>{"uid1", "key1", "uid1", "key2"}), runtime_error);
    EXPECT_THROW(keychain.untokenize(vector<string>{"uid1", "key1", "uid2"}), runtime_error);
    EXPECT_THROW(keychain.untokenize(vector<string>{"", "key1"}), runtime_error);
}

TEST(KeychainTest, TokenizationAndUntokenization) {
    Keychain keychain_1_A(map<string, string>{});
    Keychain keychain_2_A(map<string, string>{{"uid1", "key1"}});
    Keychain keychain_3_A(map<string, string>{{"uid1", "key1"}, {"uid2", "key2"}});
    Keychain keychain_4_A(map<string, string>{{"uid1", "key1"}, {"uid2", "key1"}, {"uid3", "key1"}});

    Keychain keychain_1_B;
    Keychain keychain_2_B;
    Keychain keychain_3_B;
    Keychain keychain_4_B;

    vector<string> tokens1, tokens2;

    keychain_1_A.tokenize(tokens1);
    keychain_1_B.untokenize(tokens1);
    keychain_1_B.tokenize(tokens2);
    LOG_INFO("tokens1: [" + Utils::join(tokens1, ", ") + "]");
    LOG_INFO("tokens2: [" + Utils::join(tokens2, ", ") + "]");
    EXPECT_EQ(tokens1, tokens2);
    tokens1.clear();
    tokens2.clear();

    keychain_2_A.tokenize(tokens1);
    keychain_2_B.untokenize(tokens1);
    keychain_2_B.tokenize(tokens2);
    LOG_INFO("tokens1: [" + Utils::join(tokens1, ", ") + "]");
    LOG_INFO("tokens2: [" + Utils::join(tokens2, ", ") + "]");
    EXPECT_EQ(tokens1, tokens2);
    tokens1.clear();
    tokens2.clear();

    keychain_3_A.tokenize(tokens1);
    keychain_3_B.untokenize(tokens1);
    keychain_3_B.tokenize(tokens2);
    LOG_INFO("tokens1: [" + Utils::join(tokens1, ", ") + "]");
    LOG_INFO("tokens2: [" + Utils::join(tokens2, ", ") + "]");
    EXPECT_EQ(tokens1, tokens2);
    tokens1.clear();
    tokens2.clear();

    keychain_4_A.tokenize(tokens1);
    keychain_4_B.untokenize(tokens1);
    keychain_4_B.tokenize(tokens2);
    LOG_INFO("tokens1: [" + Utils::join(tokens1, ", ") + "]");
    LOG_INFO("tokens2: [" + Utils::join(tokens2, ", ") + "]");
    EXPECT_EQ(tokens1, tokens2);
    tokens1.clear();
    tokens2.clear();
}
