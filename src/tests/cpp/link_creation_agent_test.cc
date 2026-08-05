#include <cmath>

#include "LinkCreationProcessor.h"
#include "LinkCreationProxy.h"
#include "LinkCreatorRegistry.h"
#include "Logger.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"
#include "Utils.h"
#include "gtest/gtest.h"

using namespace link_creation_agent;
using namespace link_creators;
using namespace das_test;

class TestLinkCreator : public LinkCreator {
   public:
    virtual pair<unsigned int, unsigned int> create(shared_ptr<QueryAnswer> query_answer) {
        return make_pair((unsigned int) round(query_answer->importance),
                         (unsigned int) round(query_answer->importance));
    }
};

TEST(LinkCreation, link_creator_function) {
    AtomDBSingleton::init(test_atomdb_json_config());
    init_test_system_parameters_singleton();

    string peer1_id = "localhost:40043";
    string peer2_id = "localhost:40044";
    ServiceBusSingleton::init(peer1_id, "", 40700, 40799);
    LinkCreatorRegistry::initialize_statics();
    shared_ptr<ServiceBus> query_bus = ServiceBusSingleton::get_instance();
    Utils::sleep(1000);

    auto processor = make_shared<LinkCreationProcessor>();
    shared_ptr<ServiceBus> bus = make_shared<ServiceBus>(peer2_id, peer1_id);
    Utils::sleep(1000);
    bus->register_processor(processor);

    LinkCreationProxy proxy1({""}, "link_creation_test", "unit_test");
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blah", 0.0)),
              make_pair((unsigned int) 4, (unsigned int) 4));
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blahhh", 0.5)),
              make_pair((unsigned int) 6, (unsigned int) 6));
    EXPECT_EQ(proxy1.link_creation(make_shared<QueryAnswer>("blahh", 1.0)),
              make_pair((unsigned int) 5, (unsigned int) 5));
    LinkCreationProxy proxy2({""},
                             "link_creation_test",
                             LinkCreatorRegistry::REMOTE_FUNCTION,
                             make_shared<TestLinkCreator>());
    EXPECT_EQ(proxy2.link_creation(make_shared<QueryAnswer>("blah", 0.0)),
              make_pair((unsigned int) 0, (unsigned int) 0));
    EXPECT_EQ(proxy2.link_creation(make_shared<QueryAnswer>("blahhh", 0.4)),
              make_pair((unsigned int) 0, (unsigned int) 0));
    EXPECT_EQ(proxy2.link_creation(make_shared<QueryAnswer>("blahh", 0.9)),
              make_pair((unsigned int) 1, (unsigned int) 1));
    vector<string> tokens;
    proxy2.tokenize(tokens);
    LinkCreationProxy proxy3;
    proxy3.untokenize(tokens);
    EXPECT_FALSE(proxy1.is_link_creation_function_remote());
    EXPECT_FALSE(proxy2.is_link_creation_function_remote());
    EXPECT_TRUE(proxy3.is_link_creation_function_remote());
    EXPECT_THROW(proxy3.link_creation(make_shared<QueryAnswer>("blah", 0.0)), runtime_error);
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
