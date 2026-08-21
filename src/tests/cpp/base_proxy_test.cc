#include <gtest/gtest.h>

#include "BaseProxy.h"
#include "SystemParametersSingleton.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"

using namespace std;
using namespace commons;
using namespace agents;

class TestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override { das_test::init_test_system_parameters_singleton(); }

    void TearDown() override {}
};

class TestProxy : public BaseProxy {
   public:
    void pack_command_line_args() {}
    void set_orchestration(unsigned int value) {
        set_orchestration_schema((BaseProxy::ORCHESTRATION_SCHEMA_TYPE) value);
    }
};

TEST(BaseProxyTest, basics) {
    TestProxy proxy;
    EXPECT_TRUE(proxy.cycle_start_allowed());
    EXPECT_TRUE(proxy.cycle_start_allowed());
    proxy.set_orchestration(1);
    EXPECT_FALSE(proxy.cycle_start_allowed());
    EXPECT_FALSE(proxy.cycle_start_allowed());
    proxy.allow_cycle_start({});
    EXPECT_TRUE(proxy.cycle_start_allowed());
    EXPECT_FALSE(proxy.cycle_start_allowed());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new TestEnvironment());
    return RUN_ALL_TESTS();
}
