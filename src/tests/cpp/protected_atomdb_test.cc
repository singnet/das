#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "InMemoryAccessPermissionTypes.h"
#include "InMemoryDB.h"
#include "Link.h"
#include "Node.h"
#include "ProtectedAtomDB.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace atoms;
using namespace std;

namespace {

class ProtectedInMemoryDB : public InMemoryDB {
   public:
    ProtectedInMemoryDB(const string& context = "") : InMemoryDB(context) {}

    atomdb_api_types::ProtectionMode get_protection_mode() const override {
        return atomdb_api_types::ProtectionMode::PROTECTED;
    }
};

class FullAccessProtectedInMemoryDB : public ProtectedInMemoryDB {
   public:
    explicit FullAccessProtectedInMemoryDB(const string& context, const string& allowed_key)
        : ProtectedInMemoryDB(context), allowed_key(allowed_key) {}

    vector<shared_ptr<atomdb_api_types::AccessPermissionDocument>> get_access_permissions(
        const atomdb_api_types::PublicKey& public_key) const override {
        if (!public_key.is_single_key() || public_key.keys.empty() ||
            public_key.keys[0] != this->allowed_key) {
            return {};
        }
        auto document = make_shared<InMemoryAccessPermissionDocument>();
        document->set_access_key(this->allowed_key);
        document->set_full_access(true);
        return {document};
    }

   private:
    string allowed_key;
};

shared_ptr<ProtectedAtomDB> make_protected_db(const string& context = "protected_atomdb_test_") {
    return make_shared<ProtectedAtomDB>(make_shared<ProtectedInMemoryDB>(context));
}

}  // namespace

TEST(ProtectedAtomDBTest, RejectsNullBackend) { EXPECT_THROW(ProtectedAtomDB(nullptr), runtime_error); }

TEST(ProtectedAtomDBTest, ReportsProtectionModeThroughWrapper) {
    auto backend = make_shared<ProtectedInMemoryDB>("protected_flags_");
    EXPECT_EQ(backend->get_protection_mode(), ProtectionMode::PROTECTED);

    ProtectedAtomDB db(backend);
    EXPECT_EQ(db.get_protection_mode(), ProtectionMode::PROTECTED);
}

TEST(ProtectedAtomDBTest, DelegatesToBackend) {
    auto backend = make_shared<ProtectedInMemoryDB>("protected_flags_");
    ProtectedAtomDB db(backend);
    EXPECT_EQ(db.allow_nested_indexing(), backend->allow_nested_indexing());
    EXPECT_EQ(db.composite_type_enabled(), backend->composite_type_enabled());

    PublicKey key("any_key");

    EXPECT_EQ(db.get_access_permissions(key).size(), backend->get_access_permissions(key).size());
}

TEST(ProtectedAtomDBTest, RejectsOperationsWithoutPublicKey) {
    auto db = make_protected_db();

    Node node("Symbol", "\"node\"");
    Link link("Expression", {"a", "b"});
    vector<string> handles = {"a", "b"};
    vector<Atom*> atoms = {&node};

    EXPECT_THROW(db->get_atom("handle"), runtime_error);
    EXPECT_THROW(db->get_node("handle"), runtime_error);
    EXPECT_THROW(db->get_link("handle"), runtime_error);

    EXPECT_THROW(db->get_matching_atoms(true, node), runtime_error);

    LinkSchema schema("Expression", 2);
    EXPECT_THROW(db->query_for_pattern(schema), runtime_error);
    EXPECT_THROW(db->query_for_targets("handle"), runtime_error);
    EXPECT_THROW(db->query_for_incoming_set("handle"), runtime_error);

    EXPECT_THROW(db->atom_exists("handle"), runtime_error);
    EXPECT_THROW(db->node_exists("handle"), runtime_error);
    EXPECT_THROW(db->link_exists("handle"), runtime_error);

    EXPECT_THROW(db->atoms_exist(handles), runtime_error);
    EXPECT_THROW(db->nodes_exist(handles), runtime_error);
    EXPECT_THROW(db->links_exist(handles), runtime_error);

    EXPECT_THROW(db->add_atom(&node), runtime_error);
    EXPECT_THROW(db->add_node(&node), runtime_error);
    EXPECT_THROW(db->add_link(&link), runtime_error);

    EXPECT_THROW(db->add_atoms(atoms), runtime_error);
    EXPECT_THROW(db->add_nodes({&node}), runtime_error);
    EXPECT_THROW(db->add_links({&link}), runtime_error);

    EXPECT_THROW(db->delete_atom("handle"), runtime_error);
    EXPECT_THROW(db->delete_node("handle"), runtime_error);
    EXPECT_THROW(db->delete_link("handle"), runtime_error);

    EXPECT_THROW(db->delete_atoms(handles), runtime_error);
    EXPECT_THROW(db->delete_nodes(handles), runtime_error);
    EXPECT_THROW(db->delete_links(handles), runtime_error);

    EXPECT_THROW(db->re_index_patterns(), runtime_error);

    EXPECT_THROW(db->node_count(), runtime_error);
    EXPECT_THROW(db->link_count(), runtime_error);
    EXPECT_THROW(db->atom_count(), runtime_error);
}

TEST(ProtectedAtomDBTest, GetAtomAuthorizesWithPublicKey) {
    auto backend = make_shared<FullAccessProtectedInMemoryDB>("protected_get_atom_", "admin");
    Node node("Symbol", "\"protected_node\"");
    string handle = backend->add_node(&node);

    ProtectedAtomDB db(backend);
    PublicKey admin("admin");
    PublicKey other("other");

    auto allowed = db.get_atom(handle, admin);
    ASSERT_NE(allowed, nullptr);
    EXPECT_EQ(allowed->handle(), handle);

    EXPECT_EQ(db.get_atom(handle, other), nullptr);

    Node missing_node("Symbol", "\"missing_protected_node\"");
    EXPECT_EQ(db.get_atom(missing_node.handle(), admin), nullptr);

    auto node_again = db.get_node(handle, admin);
    ASSERT_NE(node_again, nullptr);
    EXPECT_EQ(node_again->handle(), handle);
    EXPECT_EQ(db.get_node(handle, other), nullptr);
}
