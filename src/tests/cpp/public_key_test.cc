#include <map>
#include <optional>
#include <stdexcept>
#include <string>

#include "PublicKey.h"
#include "gtest/gtest.h"

using namespace atomdb;
using namespace std;

TEST(PublicKeyTest, SingleKeyConstructor) {
    PublicKey key("admin");
    EXPECT_TRUE(key.is_single_key());
    EXPECT_EQ(key.key(), "admin");
    EXPECT_TRUE(key.peer_keys().empty());
}

TEST(PublicKeyTest, SingleKeyConstructorRejectsEmpty) { EXPECT_THROW(PublicKey(""), runtime_error); }

TEST(PublicKeyTest, MappedConstructor) {
    PublicKey key(map<string, string>{{"peerA", "admin"}, {"peerB", "reader"}});
    EXPECT_FALSE(key.is_single_key());
    EXPECT_EQ(key.peer_keys().size(), 2u);
    EXPECT_EQ(key.peer_keys()["peerA"], "admin");
    EXPECT_EQ(key.peer_keys()["peerB"], "reader");
}

TEST(PublicKeyTest, MappedConstructorRejectsEmptyMap) {
    EXPECT_THROW(PublicKey(map<string, string>{}), runtime_error);
}

TEST(PublicKeyTest, MappedConstructorRejectsEmptyPeerOrKey) {
    EXPECT_THROW(PublicKey(map<string, string>{{"", "admin"}}), runtime_error);
    EXPECT_THROW(PublicKey(map<string, string>{{"peerA", ""}}), runtime_error);
}

TEST(PublicKeyTest, KeyForPeerBroadcastsSingleKey) {
    PublicKey key("admin");
    EXPECT_EQ(key.key_for_peer("peerA"), "admin");
    EXPECT_EQ(key.key_for_peer("missing"), "admin");
}

TEST(PublicKeyTest, KeyForPeerLooksUpMappedKey) {
    PublicKey key(map<string, string>{{"peerA", "admin"}, {"peerB", "reader"}});
    EXPECT_EQ(key.key_for_peer("peerA"), "admin");
    EXPECT_EQ(key.key_for_peer("peerB"), "reader");
    EXPECT_FALSE(key.key_for_peer("missing").has_value());
}

TEST(PublicKeyTest, ForPeerReturnsSelfWhenSingleKey) {
    PublicKey key("admin");
    optional<PublicKey> sliced = key.for_peer("any");
    ASSERT_TRUE(sliced.has_value());
    EXPECT_TRUE(sliced->is_single_key());
    EXPECT_EQ(sliced->key(), "admin");
}

TEST(PublicKeyTest, ForPeerSlicesMappedKey) {
    PublicKey key(map<string, string>{{"peerA", "admin"}, {"peerB", "reader"}});
    optional<PublicKey> sliced = key.for_peer("peerA");
    ASSERT_TRUE(sliced.has_value());
    EXPECT_TRUE(sliced->is_single_key());
    EXPECT_EQ(sliced->key(), "admin");
    EXPECT_FALSE(key.for_peer("missing").has_value());
}

TEST(PublicKeyTest, FromJsonAcceptsObjectAndString) {
    optional<PublicKey> mapped = PublicKey::from_json(R"({"peerA":"admin","peerB":"unused"})");
    ASSERT_TRUE(mapped.has_value());
    EXPECT_FALSE(mapped->is_single_key());
    EXPECT_EQ(mapped->peer_keys().size(), 2u);
    EXPECT_EQ(mapped->key_for_peer("peerA"), "admin");
    EXPECT_EQ(mapped->key_for_peer("peerB"), "unused");

    optional<PublicKey> single = PublicKey::from_json(R"("admin")");
    ASSERT_TRUE(single.has_value());
    EXPECT_TRUE(single->is_single_key());
    EXPECT_EQ(single->key(), "admin");
}

TEST(PublicKeyTest, FromJsonEmptyObjectIsAbsent) {
    EXPECT_FALSE(PublicKey::from_json("{}").has_value());
}

TEST(PublicKeyTest, FromJsonRejectsInvalidPayloads) {
    EXPECT_THROW(PublicKey::from_json("not-json"), runtime_error);
    EXPECT_THROW(PublicKey::from_json("[]"), runtime_error);
    EXPECT_THROW(PublicKey::from_json("1"), runtime_error);
    EXPECT_THROW(PublicKey::from_json("null"), runtime_error);
    EXPECT_THROW(PublicKey::from_json(R"({"peerA":1})"), runtime_error);
    EXPECT_THROW(PublicKey::from_json(R"(")"), runtime_error);
    EXPECT_THROW(PublicKey::from_json(R"("")"), runtime_error);
}
