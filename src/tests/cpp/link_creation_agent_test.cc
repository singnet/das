#include <cmath>

#include "LinkCreationProcessor.h"
#include "LinkCreationProxy.h"
#include "LinkCreatorRegistry.h"
#include "Logger.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "InMemoryDB.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"
#include "Utils.h"
#include "gtest/gtest.h"

using namespace link_creation_agent;
using namespace link_creators;
using namespace das_test;

class TestLinkCreator : public LinkCreator {
   public:
    virtual LinkCreationStats create(shared_ptr<QueryAnswer> query_answer) {
        return LinkCreationStats(false, round(query_answer->importance), 0);
    }
};

TEST(LinkCreation, link_creator_function) {
    LinkCreationProxy proxy1({""}, "link_creation_test", "unit_test");
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blah", 0.0)).created, 4);
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blahhh", 0.5)).created, 6);
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blahh", 1.0)).created, 5);
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blah", 0.0)).updated, 5);
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blahhh", 0.5)).updated, 7);
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blahh", 1.0)).updated, 6);
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blah", 0.0)).visited, true);
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blahhh", 0.5)).visited, true);
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blahh", 1.0)).visited, true);

    EXPECT_THROW(LinkCreationProxy proxy2({""},
                                          "link_creation_test",
                                          LinkCreatorRegistry::REMOTE_FUNCTION,
                                          make_shared<TestLinkCreator>()),
                 runtime_error);
}

TEST(LinkCreation, proxy_object) {
    LinkCreationProxy proxy({"t0", "t1"}, "context", "unit_test");
    vector<string> tokens1, tokens2, tokens3;
    proxy.tokenize(tokens1);
    tokens2 = tokens1;
    LinkCreationProxy proxy2;
    proxy2.untokenize(tokens2);
    proxy2.tokenize(tokens3);
    cout << "tokens1: " << Utils::join(tokens1) << endl;
    cout << "tokens3: " << Utils::join(tokens3) << endl;
    EXPECT_EQ(tokens1, tokens3);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    AtomDBSingleton::provide(make_shared<InMemoryDB>());
    init_test_system_parameters_singleton();
    LinkCreatorRegistry::initialize_statics();

    //string peer1_id = "localhost:40048";
    //string peer2_id = "localhost:40049";
    //ServiceBusSingleton::init(peer1_id, "", 41800, 41899);
    //FitnessFunctionRegistry::initialize_statics();
    //shared_ptr<ServiceBus> query_bus = ServiceBusSingleton::get_instance();
    //query_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());
    //Utils::sleep(1000);

    //auto processor = make_shared<TestProcessor>();
    //shared_ptr<ServiceBus> bus = make_shared<ServiceBus>(peer2_id, peer1_id);
    //Utils::sleep(1000);
    //bus->register_processor(processor);


    return RUN_ALL_TESTS();
}

