#include <cmath>
#include <cstdlib>

#include "LeadershipBroker.h"
#include "gtest/gtest.h"

using namespace distributed_algorithm_node;

TEST(LeadershipBroker, basics) {
    try {
        LeadershipBroker::factory((LeadershipBrokerType) -1);
        FAIL() << "Expected exception";
    } catch (std::runtime_error const& error) {
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }

    shared_ptr<LeadershipBroker> leadership_broker =
        LeadershipBroker::factory(LeadershipBrokerType::SINGLE_MASTER_SERVER);

    EXPECT_EQ(leadership_broker->leader_id(), "");
    EXPECT_FALSE(leadership_broker->has_leader());
    leadership_broker->start_leader_election("blah");
    EXPECT_EQ(leadership_broker->leader_id(), "blah");
    EXPECT_TRUE(leadership_broker->has_leader());
}
