#include "Keychain.h"

#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace atomdb;
using namespace std;

TEST(KeychainTest, GetPublicKeyReturnsEmpty) {
    Keychain empty_keychain1(map<string, string>{});
    EXPECT_EQ(empty_keychain1.get_public_key("blah"), "");
    EXPECT_EQ(empty_keychain1.get_public_key(""), "");

    Keychain empty_keychain2(vector<string>{});
    EXPECT_EQ(empty_keychain2.get_public_key("blah"), "");
    EXPECT_EQ(empty_keychain2.get_public_key(""), "");

    Keychain keychain1(map<string, string>{{"uid1", "key1"}});
    EXPECT_EQ(keychain1.get_public_key("uid2"), "");
    EXPECT_EQ(keychain1.get_public_key("uid"), "");
    EXPECT_EQ(keychain1.get_public_key(""), "");

    Keychain keychain2(vector<string>{"uid1", "key1"});
    EXPECT_EQ(keychain2.get_public_key("uid2"), "");
    EXPECT_EQ(keychain2.get_public_key("uid"), "");
    EXPECT_EQ(keychain2.get_public_key(""), "");

    Keychain empty_stored_key1(map<string, string>{{"uid", ""}});
    EXPECT_EQ(empty_stored_key1.get_public_key("uid"), "");

    Keychain empty_stored_key2(vector<string>{"uid", ""});
    EXPECT_EQ(empty_stored_key2.get_public_key("uid"), "");
}

TEST(KeychainTest, GetPublicKeyReturnsStoredKey) {
    Keychain keychain1(map<string, string>{{"uid1", "key1"}, {"uid2", "key2"}});
    EXPECT_EQ(keychain1.get_public_key("uid1"), "key1");
    EXPECT_EQ(keychain1.get_public_key("uid2"), "key2");

    Keychain keychain2(vector<string>{"uid1", "key1", "uid2", "key2"});
    EXPECT_EQ(keychain2.get_public_key("uid1"), "key1");
    EXPECT_EQ(keychain2.get_public_key("uid2"), "key2");
}

TEST(KeychainTest, MalformedInitializationVector) {
    EXPECT_THROW(Keychain keychain(vector<string>{"uid1"}), runtime_error);
    EXPECT_THROW(Keychain keychain(vector<string>{"uid1", "key1", "uid1", "key2"}), runtime_error);
    EXPECT_THROW(Keychain keychain(vector<string>{"uid1", "key1", "uid2"}), runtime_error);
    EXPECT_THROW(Keychain keychain(vector<string>{"", "key1"}), runtime_error);
}
