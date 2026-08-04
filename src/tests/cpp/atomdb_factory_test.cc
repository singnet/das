#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "AtomDBFactory.h"
#include "InMemoryDB.h"
#include "JsonConfig.h"
#include "MockAtomDB.h"
#include "ProtectedAtomDB.h"

using namespace atomdb;
using namespace commons;
using namespace std;
using ::testing::Return;

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
    EXPECT_EQ(dynamic_pointer_cast<ProtectedAtomDB>(backend), nullptr);
    EXPECT_FALSE(backend->is_protected());
}

TEST(AtomDBFactoryTest, CreateInMemoryDBDoesNotWrap) {
    auto db = AtomDBFactory::create(config_with_type("inmemorydb"), "factory_test_");
    ASSERT_NE(db, nullptr);
    EXPECT_NE(dynamic_pointer_cast<InMemoryDB>(db), nullptr);
    EXPECT_EQ(dynamic_pointer_cast<ProtectedAtomDB>(db), nullptr);
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

TEST(AtomDBFactoryTest, WrapIfProtectedUnprotectedReturnsSameInstance) {
    auto backend = make_shared<AtomDBMock>();
    EXPECT_CALL(*backend, is_protected()).WillRepeatedly(Return(false));

    auto wrapped = AtomDBFactory::wrap_if_protected(backend);
    EXPECT_EQ(wrapped.get(), backend.get());
}

TEST(AtomDBFactoryTest, WrapIfProtectedWrapsOnce) {
    auto backend = make_shared<AtomDBMock>();
    EXPECT_CALL(*backend, is_protected()).WillRepeatedly(Return(true));

    auto wrapped = AtomDBFactory::wrap_if_protected(backend);
    ASSERT_NE(wrapped, nullptr);
    EXPECT_NE(wrapped.get(), backend.get());
    EXPECT_NE(dynamic_pointer_cast<ProtectedAtomDB>(wrapped), nullptr);
    EXPECT_TRUE(wrapped->is_protected());

    auto wrapped_again = AtomDBFactory::wrap_if_protected(wrapped);
    EXPECT_EQ(wrapped_again.get(), wrapped.get());
}
