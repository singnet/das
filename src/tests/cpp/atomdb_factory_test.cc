#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "AtomDBFactory.h"
#include "InMemoryDB.h"
#include "JsonConfig.h"
#include "MockAtomDB.h"
#include "MorkDB.h"
#include "ProtectedAtomDB.h"
#include "TestAtomDBJsonConfig.h"

using namespace atomdb;
using namespace commons;
using namespace std;

namespace {

JsonConfig config_with_type(const string& type) {
    JsonConfig config;
    config["type"] = type;
    return config;
}

}  // namespace

TEST(AtomDBFactoryTest, CreateBackendInMemoryDB) {
    auto backend = AtomDBFactory::create_backend(config_with_type("inmemorydb"), "factory_test_");
    ASSERT_NE(backend, nullptr);
    EXPECT_NE(dynamic_pointer_cast<InMemoryDB>(backend), nullptr);
}

TEST(AtomDBFactoryTest, CreateInMemoryDB) {
    auto db = AtomDBFactory::create(config_with_type("inmemorydb"), "factory_test_");
    ASSERT_NE(db, nullptr);
    EXPECT_NE(dynamic_pointer_cast<InMemoryDB>(db), nullptr);
}

TEST(AtomDBFactoryTest, CreateBackendMorkDB) {
    auto backend =
        AtomDBFactory::create_backend(test_atomdb_json_config("morkdb"), "factory_mork_backend_");
    ASSERT_NE(backend, nullptr);
    EXPECT_NE(dynamic_pointer_cast<MorkDB>(backend), nullptr);
}

TEST(AtomDBFactoryTest, CreateMorkDB) {
    auto db = AtomDBFactory::create(test_atomdb_json_config("morkdb"), "factory_mork_create_");
    ASSERT_NE(db, nullptr);
    EXPECT_NE(dynamic_pointer_cast<MorkDB>(db), nullptr);
}

TEST(AtomDBFactoryTest, CreateAndCreateBackendAreCompatibleForMorkDB) {
    auto config = test_atomdb_json_config("morkdb");
    auto backend = AtomDBFactory::create_backend(config, "factory_mork_compat_");
    auto created = AtomDBFactory::create(config, "factory_mork_compat_");

    ASSERT_NE(backend, nullptr);
    ASSERT_NE(created, nullptr);
    EXPECT_NE(dynamic_pointer_cast<MorkDB>(backend), nullptr);
    // create() only wraps when the backend is protected.
    if (backend->is_protected()) {
        EXPECT_NE(dynamic_pointer_cast<ProtectedAtomDB>(created), nullptr);
        EXPECT_TRUE(created->is_protected());
    } else {
        EXPECT_NE(dynamic_pointer_cast<MorkDB>(created), nullptr);
        EXPECT_EQ(AtomDBFactory::wrap_if_protected(backend).get(), backend.get());
    }
}

TEST(AtomDBFactoryTest, CreateBackendRejectsMissingAndUnknownTypes) {
    EXPECT_THROW(AtomDBFactory::create_backend(JsonConfig()), runtime_error);
    EXPECT_THROW(AtomDBFactory::create_backend(config_with_type("")), runtime_error);
    EXPECT_THROW(AtomDBFactory::create_backend(config_with_type("unknown")), runtime_error);
    EXPECT_THROW(AtomDBFactory::create_backend(config_with_type("remotedb")), runtime_error);
    EXPECT_THROW(AtomDBFactory::create_backend(config_with_type("adapterdb")), runtime_error);
}

TEST(AtomDBFactoryTest, WrapIfProtectedNullThrows) {
    EXPECT_THROW(AtomDBFactory::wrap_if_protected(nullptr), runtime_error);
}

TEST(AtomDBFactoryTest, WrapIfProtectedLeavesUnprotectedBackend) {
    auto backend = make_shared<AtomDBMock>();

    auto wrapped = AtomDBFactory::wrap_if_protected(backend);
    EXPECT_EQ(wrapped.get(), backend.get());
    EXPECT_FALSE(wrapped->is_protected());
}

TEST(AtomDBFactoryTest, WrapIfProtectedWrapsProtectedBackend) {
    auto backend = make_shared<::testing::NiceMock<AtomDBMock>>();
    EXPECT_CALL(*backend, is_protected()).WillRepeatedly(::testing::Return(true));

    auto wrapped = AtomDBFactory::wrap_if_protected(backend);
    ASSERT_NE(wrapped, nullptr);
    EXPECT_NE(wrapped.get(), backend.get());
    EXPECT_NE(dynamic_pointer_cast<ProtectedAtomDB>(wrapped), nullptr);
    EXPECT_TRUE(wrapped->is_protected());
}

TEST(AtomDBFactoryTest, WrapIfProtectedIsIdempotent) {
    auto backend = make_shared<::testing::NiceMock<AtomDBMock>>();
    EXPECT_CALL(*backend, is_protected()).WillRepeatedly(::testing::Return(true));

    auto wrapped = AtomDBFactory::wrap_if_protected(backend);
    ASSERT_NE(wrapped, nullptr);
    EXPECT_NE(dynamic_pointer_cast<ProtectedAtomDB>(wrapped), nullptr);

    auto wrapped_again = AtomDBFactory::wrap_if_protected(wrapped);
    EXPECT_EQ(wrapped_again.get(), wrapped.get());
}

TEST(AtomDBFactoryTest, CreateInMemoryDBIsNotProtected) {
    auto db = AtomDBFactory::create(config_with_type("inmemorydb"), "factory_unprotected_");
    ASSERT_NE(db, nullptr);
    EXPECT_FALSE(db->is_protected());
    EXPECT_EQ(dynamic_pointer_cast<ProtectedAtomDB>(db), nullptr);
}
