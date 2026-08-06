#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "InMemoryDB.h"
#include "Link.h"
#include "Node.h"
#include "ProtectedAtomDB.h"

using namespace atomdb;
using namespace atoms;
using namespace std;

namespace {

shared_ptr<ProtectedAtomDB> make_protected_db(const string& context = "protected_atomdb_test_") {
    return make_shared<ProtectedAtomDB>(make_shared<InMemoryDB>(context));
}

}  // namespace

TEST(ProtectedAtomDBTest, RejectsNullBackend) { EXPECT_THROW(ProtectedAtomDB(nullptr), runtime_error); }

TEST(ProtectedAtomDBTest, IsProtected) {
    auto db = make_protected_db();
    EXPECT_TRUE(db->is_protected());
}

TEST(ProtectedAtomDBTest, BackendIsNotProtected) {
    auto backend = make_shared<InMemoryDB>("protected_backend_");
    EXPECT_FALSE(backend->is_protected());
}

TEST(ProtectedAtomDBTest, ExposesWrappedBackend) {
    auto backend = make_shared<InMemoryDB>("protected_backend_access_");
    ProtectedAtomDB db(backend);

    EXPECT_EQ(db.get_backend().get(), backend.get());
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
    const string key = "public_key";

    EXPECT_THROW(db->get_atom("handle", key), runtime_error);
    EXPECT_THROW(db->atom_exists("handle", key), runtime_error);
    EXPECT_THROW(db->add_node(&node, key), runtime_error);
    EXPECT_THROW(db->delete_atom("handle", key), runtime_error);
    EXPECT_THROW(db->atom_count(key), runtime_error);
}
