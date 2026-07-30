#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "AtomDBSingleton.h"
#include "MockAtomDB.h"
#include "ProtectedAtomDB.h"
#include "RemoteAtomDB.h"
#include "TestAtomDBJsonConfig.h"

using namespace atomdb;
using namespace commons;
using namespace std;
using namespace testing;

shared_ptr<AtomDB> wrap_if_protected(shared_ptr<AtomDB> backend, const JsonConfig& config) {
    if (backend->is_protected()) {
        return make_shared<ProtectedAtomDB>(backend, config);
    }
    return backend;
}

TEST(AtomDBSingletonTest, InitWrapsProtectedBackend) {
    auto mock = make_shared<AtomDBMock>();
    EXPECT_CALL(*mock, is_protected()).WillRepeatedly(Return(true));

    auto wrapped = wrap_if_protected(mock, test_atomdb_json_config());
    EXPECT_NE(dynamic_cast<ProtectedAtomDB*>(wrapped.get()), nullptr);
    EXPECT_TRUE(wrapped->is_protected());
}

TEST(AtomDBSingletonTest, InitAndProvideBehavior) {
    nlohmann::json json;
    json["type"] = "remotedb";
    json["remote_peers"] = nlohmann::json::array(
        {{{"uid", "peer1"}, {"type", "inmemorydb"}, {"context", "atomdb_singleton_test_"}}});
    AtomDBSingleton::init(JsonConfig(json));

    auto db = AtomDBSingleton::get_instance();
    EXPECT_EQ(dynamic_cast<ProtectedAtomDB*>(db.get()), nullptr);
    EXPECT_NE(dynamic_cast<RemoteAtomDB*>(db.get()), nullptr);
    EXPECT_FALSE(db->is_protected());

    auto mock = make_shared<AtomDBMock>();
    EXPECT_CALL(*mock, is_protected()).WillRepeatedly(Return(true));
    AtomDBSingleton::provide(mock);

    db = AtomDBSingleton::get_instance();
    EXPECT_EQ(db.get(), mock.get());
    EXPECT_EQ(dynamic_cast<ProtectedAtomDB*>(db.get()), nullptr);

    AtomDBSingleton::provide(nullptr);
}
