#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "AtomDBSingleton.h"
#include "MockAtomDB.h"
#include "ProtectedAtomDB.h"
#include "RedisMongoDB.h"
#include "RemoteAtomDB.h"
#include "TestAtomDBJsonConfig.h"

using namespace atomdb;
using namespace commons;
using namespace std;
using namespace testing;

TEST(AtomDBSingletonTest, InitWrapsProtectedBackend) {
    auto config = test_atomdb_json_config();
    config["mongodb"]["seed_protected"] = true;
    AtomDBSingleton::init(config);

    auto db = AtomDBSingleton::get_instance();
    ASSERT_NE(db, nullptr);
    EXPECT_NE(dynamic_cast<ProtectedAtomDB*>(db.get()), nullptr);
    EXPECT_TRUE(db->is_protected());

    {
        auto cleanup = make_shared<RedisMongoDB>("", false, test_atomdb_json_config());
        cleanup->drop_all();
    }
    AtomDBSingleton::provide(nullptr);
}

TEST(AtomDBSingletonTest, InitAndProvideBehavior) {
    nlohmann::json json;
    json["remote_peers"] = nlohmann::json::array(
        {{{"uid", "peer1"}, {"type", "inmemorydb"}, {"context", "atomdb_singleton_test_"}}});
    AtomDBSingleton::provide(make_shared<RemoteAtomDB>(JsonConfig(json["remote_peers"])));

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
