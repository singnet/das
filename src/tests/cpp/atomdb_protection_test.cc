#include <gtest/gtest.h>

#include <memory>

#include "AtomDBFactory.h"
#include "AtomDBSingleton.h"
#include "InMemoryDB.h"
#include "MockAtomDB.h"
#include "ProtectedAtomDB.h"

using namespace atomdb;
using namespace std;
using ::testing::Return;

namespace {

void reset_singleton() { AtomDBSingleton::provide(make_shared<InMemoryDB>("protection_test_")); }

}  // namespace

class AtomDBSingletonProtectionTest : public ::testing::Test {
   protected:
    void TearDown() override { reset_singleton(); }
};

TEST_F(AtomDBSingletonProtectionTest, WrapIfProtectedThenProvide) {
    auto backend = make_shared<AtomDBMock>();
    EXPECT_CALL(*backend, is_protected()).WillRepeatedly(Return(true));

    AtomDBSingleton::provide(AtomDBFactory::wrap_if_protected(backend));
    auto instance = AtomDBSingleton::get_instance();
    ASSERT_NE(dynamic_pointer_cast<ProtectedAtomDB>(instance), nullptr);
    EXPECT_TRUE(instance->is_protected());
}

TEST_F(AtomDBSingletonProtectionTest, ProvideUnprotectedKeepsBackendUnwrapped) {
    auto backend = make_shared<AtomDBMock>();
    EXPECT_CALL(*backend, is_protected()).WillRepeatedly(Return(false));

    AtomDBSingleton::provide(AtomDBFactory::wrap_if_protected(backend));
    auto instance = AtomDBSingleton::get_instance();
    EXPECT_EQ(instance.get(), backend.get());
    EXPECT_EQ(dynamic_pointer_cast<ProtectedAtomDB>(instance), nullptr);
}

TEST(ProtectedAtomDBAccess, KeylessAccessRequiresPublicKey) {
    auto backend = make_shared<AtomDBMock>();
    auto db = make_shared<ProtectedAtomDB>(backend);

    EXPECT_THROW(db->get_atom("handle"), runtime_error);
    EXPECT_THROW(db->atom_exists("handle"), runtime_error);
    EXPECT_THROW(db->node_count(), runtime_error);
}

TEST(ProtectedAtomDBAccess, AuthorizedOverloadsNotImplementedYet) {
    auto backend = make_shared<AtomDBMock>();
    auto db = make_shared<ProtectedAtomDB>(backend);

    EXPECT_THROW(db->get_atom("handle", "public_key"), runtime_error);
    EXPECT_THROW(db->atom_exists("handle", "public_key"), runtime_error);
    EXPECT_THROW(db->node_count("public_key"), runtime_error);
}
