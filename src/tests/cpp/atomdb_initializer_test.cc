#include <gtest/gtest.h>

#include "AtomDBInitializer.h"
#include "JsonConfig.h"

using namespace atomdb;
using namespace commons;
using namespace std;

namespace {

JsonConfig inmemory_config() {
    JsonConfig config;
    config["type"] = "inmemorydb";
    config["uid"] = "atomdb_initializer_test";
    return config;
}

}  // namespace

TEST(AtomDBInitializerTest, InitRaisesOnSecondCall) {
    AtomDBInitializer::init(inmemory_config());
    EXPECT_THROW(AtomDBInitializer::init(inmemory_config()), runtime_error);
}
