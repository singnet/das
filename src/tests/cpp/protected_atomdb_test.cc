#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "InMemoryDB.h"
#include "Link.h"
#include "Node.h"
#include "ProtectedAtomDB.h"
#include "PublicKey.h"

using namespace atomdb;
using namespace atoms;
using namespace std;

using atomdb_api_types::ProtectionMode;

namespace {

shared_ptr<ProtectedAtomDB> make_protected_db(const string& context = "protected_atomdb_test_") {
    return make_shared<ProtectedAtomDB>(make_shared<InMemoryDB>(context));
}

}  // namespace

TEST(PublicKeyTest, SingleKey) {
    PublicKey key("pk1");
    EXPECT_TRUE(key.is_single_key());
    EXPECT_FALSE(key.is_peer_map());
    EXPECT_EQ(key.key(), "pk1");
    EXPECT_THROW(key.keys(), runtime_error);
}

TEST(PublicKeyTest, PeerMap) {
    PublicKey key(map<string, string>{{"peer1", "k1"}, {"peer2", "k2"}});
    EXPECT_FALSE(key.is_single_key());
    EXPECT_TRUE(key.is_peer_map());
    EXPECT_EQ(key.keys().size(), 2u);
    EXPECT_EQ(key.keys().at("peer1"), "k1");
    EXPECT_THROW(key.key(), runtime_error);
}

TEST(PublicKeyTest, RejectsEmpty) {
    EXPECT_THROW(PublicKey(""), runtime_error);
    EXPECT_THROW(PublicKey(map<string, string>{}), runtime_error);
    EXPECT_THROW(PublicKey(map<string, string>{{"", "k"}}), runtime_error);
    EXPECT_THROW(PublicKey(map<string, string>{{"peer", ""}}), runtime_error);
}

TEST(ProtectedAtomDBTest, RejectsNullBackend) { EXPECT_THROW(ProtectedAtomDB(nullptr), runtime_error); }

TEST(ProtectedAtomDBTest, IsProtected) {
    auto db = make_protected_db();
    EXPECT_EQ(db->is_protected(), ProtectionMode::PROTECTED);
}

TEST(ProtectedAtomDBTest, BackendIsNotProtected) {
    auto backend = make_shared<InMemoryDB>("protected_backend_");
    EXPECT_EQ(backend->is_protected(), ProtectionMode::UNPROTECTED);
}

TEST(ProtectedAtomDBTest, ForwardModeWhenBackendIsForward) {
    class ForwardInMemoryDB : public InMemoryDB {
       public:
        explicit ForwardInMemoryDB(const string& context) : InMemoryDB(context) {}
        ProtectionMode is_protected() const override { return ProtectionMode::FORWARD; }
    };

    ProtectedAtomDB db(make_shared<ForwardInMemoryDB>("protected_forward_"));
    EXPECT_EQ(db.is_protected(), ProtectionMode::FORWARD);
}

TEST(ProtectedAtomDBTest, DelegatesCapabilityFlagsToBackend) {
    auto backend = make_shared<InMemoryDB>("protected_flags_");
    ProtectedAtomDB db(backend);

    EXPECT_EQ(db.allow_nested_indexing(), backend->allow_nested_indexing());
    EXPECT_EQ(db.composite_type_enabled(), backend->composite_type_enabled());
}

TEST(ProtectedAtomDBTest, RejectsAccessWithoutPublicKey) {
    auto db = make_protected_db("protected_no_key_");
    Node node("Symbol", "\"x\"");
    Link link("Expression", {"a", "b"});

    EXPECT_THROW(db->get_atom("handle"), runtime_error);
    EXPECT_THROW(db->atom_exists("handle"), runtime_error);
    EXPECT_THROW(db->add_node(&node), runtime_error);
    EXPECT_THROW(db->add_link(&link), runtime_error);
    EXPECT_THROW(db->delete_atom("handle"), runtime_error);
    EXPECT_THROW(db->atom_count(), runtime_error);
}

TEST(ProtectedAtomDBTest, PublicKeyOverloadsAreNotImplementedYet) {
    auto db = make_protected_db("protected_with_key_");
    Node node("Symbol", "\"n\"");
    PublicKey key("public_key");

    EXPECT_THROW(db->get_atom("handle", key), runtime_error);
    EXPECT_THROW(db->atom_exists("handle", key), runtime_error);
    EXPECT_THROW(db->add_node(&node, key), runtime_error);
    EXPECT_THROW(db->delete_atom("handle", key), runtime_error);
    EXPECT_THROW(db->atom_count(key), runtime_error);
}
