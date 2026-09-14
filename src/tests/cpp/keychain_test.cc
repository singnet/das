#include "Keychain.h"

#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace atomdb;
using namespace std;

TEST(KeychainTest, GetPublicKeyReturnsEmpty) {
    Keychain empty_keychain({});
    EXPECT_EQ(empty_keychain.get_public_key("blah"), "");
    EXPECT_EQ(empty_keychain.get_public_key(""), "");

    Keychain keychain(map<string, string>{{"uid1", "key1"}});
    EXPECT_EQ(keychain.get_public_key("uid2"), "");
    EXPECT_EQ(keychain.get_public_key("uid"), "");
    EXPECT_EQ(keychain.get_public_key(""), "");

    Keychain empty_stored_key(map<string, string>{{"uid", ""}});
    EXPECT_EQ(empty_stored_key.get_public_key("uid"), "");
}

TEST(KeychainTest, GetPublicKeyReturnsStoredKey) {
    Keychain keychain(map<string, string>{{"uid1", "key1"}, {"uid2", "key2"}});
    EXPECT_EQ(keychain.get_public_key("uid1"), "key1");
    EXPECT_EQ(keychain.get_public_key("uid2"), "key2");
}
