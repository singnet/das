#include <cmath>

#include "AndTwoPredicates.h"
#include "CustomizableLinkCreator.h"
#include "InMemoryDB.h"
#include "LinkCreationProcessor.h"
#include "LinkCreationProxy.h"
#include "LinkCreatorRegistry.h"
#include "Logger.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"
#include "UnitTestLinkCreator.h"
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
    STACK_TRACE();
    LinkCreationProxy proxy1({""}, "link_creation_test", "unit_test", BaseProxy::NONE);
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
                                          BaseProxy::NONE,
                                          make_shared<TestLinkCreator>()),
                 runtime_error);
}

TEST(LinkCreation, proxy_object) {
    STACK_TRACE();
    LinkCreationProxy proxy({"t0", "t1"}, "context", "unit_test", BaseProxy::SYNC_ON_CYCLE_START);
    proxy.parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned int) 2;

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

TEST(LinkCreation, link_creator_registry) {
    STACK_TRACE();
    ASSERT_TRUE(dynamic_pointer_cast<UnitTestLinkCreator>(
                    LinkCreatorRegistry::function(LinkCreatorRegistry::UNIT_TEST)) != nullptr);
    ASSERT_TRUE(dynamic_pointer_cast<CustomizableLinkCreator>(
                    LinkCreatorRegistry::function(LinkCreatorRegistry::CUSTOMIZABLE)) != nullptr);
    ASSERT_TRUE(dynamic_pointer_cast<AndTwoPredicates>(
                    LinkCreatorRegistry::function(LinkCreatorRegistry::AND_TWO_PREDICATES)) != nullptr);
}

TEST(LinkCreation, customizable_tokenization) {
    STACK_TRACE();
    vector<CustomizableLinkCreator> original;
    vector<CustomizableLinkCreator> copy1;
    vector<CustomizableLinkCreator> copy2;
    unsigned int count = 0;

    original.emplace_back();
    original[count++].add_link_specification({QueryAnswerElement(1), QueryAnswerElement(2)},
                                             {QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                             " type0 ",
                                             CustomizableLinkCreator::INTERSECTION_OVER_UNION,
                                             {"blah", "bleh  blih"});

    original.emplace_back();
    original[count++].add_link_specification({QueryAnswerElement(1)},
                                             {QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                             "type0",
                                             CustomizableLinkCreator::PRODUCT);

    original.emplace_back();
    original[count++].add_link_specification(
        {QueryAnswerElement(1), QueryAnswerElement(2)}, {}, "type0", CustomizableLinkCreator::PRODUCT);

    original.emplace_back();
    original[count++].add_link_specification(
        {QueryAnswerElement(1)}, {}, "blah", (CustomizableLinkCreator::StrengthComposition) 0);

    vector<string> tokens1, tokens2, tokens3;
    for (unsigned int i = 0; i < count; i++) {
        copy1.emplace_back();
        copy2.emplace_back();
        original[i].tokenize(tokens1);
        string tokens_string =
            Utils::join(tokens1, CustomizableLinkCreator::EXTRA_PARAMETERS_SPLIT_CHAR);
        LOG_INFO("tokens_string: <" + tokens_string + ">");
        copy1[i].untokenize(tokens1);
        copy1[i].tokenize(tokens2);
        copy2[i].extra_parameters(tokens_string);
        copy2[i].tokenize(tokens3);
        if (i == 0) {
            ASSERT_EQ(tokens_string, "1,2,_1,_2,2,$v1,$v2,type0,2,2,blah,bleh  blih");
        }
        ASSERT_EQ(tokens1, tokens2);
        ASSERT_EQ(tokens1, tokens3);
        tokens1.clear();
        tokens2.clear();
        tokens3.clear();
    }
}

TEST(LinkCreation, customizable_link_specification) {
    STACK_TRACE();
    unsigned int count = 0;
    vector<CustomizableLinkCreator> specs;
    specs.emplace_back();
    EXPECT_THROW(specs[count++].add_link_specification(
                     {}, {}, "", (CustomizableLinkCreator::StrengthComposition) 0),
                 runtime_error);
    specs.emplace_back();
    EXPECT_THROW(specs[count++].add_link_specification(
                     {}, {}, " ", (CustomizableLinkCreator::StrengthComposition) 0),
                 runtime_error);
    specs.emplace_back();
    EXPECT_THROW(specs[count++].add_link_specification(
                     {}, {}, "  ", (CustomizableLinkCreator::StrengthComposition) 0),
                 runtime_error);

    specs.emplace_back();
    specs[count++].add_link_specification({QueryAnswerElement(1), QueryAnswerElement(2)},
                                          {QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                          "link-type",
                                          CustomizableLinkCreator::INTERSECTION_OVER_UNION,
                                          {"query1", "query2"});
    specs.emplace_back();
    EXPECT_THROW(
        specs[count++].add_link_specification({QueryAnswerElement(2)},
                                              {QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                              "link-type",
                                              CustomizableLinkCreator::INTERSECTION_OVER_UNION,
                                              {"query1", "query2"}),
        runtime_error);
    specs.emplace_back();
    EXPECT_THROW(specs[count++].add_link_specification(
                     {QueryAnswerElement(1), QueryAnswerElement(2)},
                     {QueryAnswerElement("v1"), QueryAnswerElement("v2"), QueryAnswerElement("v3")},
                     "link-type",
                     CustomizableLinkCreator::INTERSECTION_OVER_UNION,
                     {"query1", "query2"}),
                 runtime_error);
    specs.emplace_back();
    EXPECT_THROW(
        specs[count++].add_link_specification({QueryAnswerElement(1), QueryAnswerElement(2)},
                                              {QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                              "link-type",
                                              CustomizableLinkCreator::INTERSECTION_OVER_UNION,
                                              {}),
        runtime_error);
}

int main(int argc, char** argv) {
    STACK_TRACE();
    ::testing::InitGoogleTest(&argc, argv);
    AtomDBSingleton::provide(make_shared<InMemoryDB>());
    init_test_system_parameters_singleton();
    LinkCreatorRegistry::initialize_statics();

    // string peer1_id = "localhost:40048";
    // string peer2_id = "localhost:40049";
    // ServiceBusSingleton::init(peer1_id, "", 41800, 41899);
    // FitnessFunctionRegistry::initialize_statics();
    // shared_ptr<ServiceBus> query_bus = ServiceBusSingleton::get_instance();
    // query_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());
    // Utils::sleep(1000);

    // auto processor = make_shared<TestProcessor>();
    // shared_ptr<ServiceBus> bus = make_shared<ServiceBus>(peer2_id, peer1_id);
    // Utils::sleep(1000);
    // bus->register_processor(processor);

    return RUN_ALL_TESTS();
}
