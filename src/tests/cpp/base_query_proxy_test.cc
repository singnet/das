#include <gtest/gtest.h>

#include "AtomDBSingleton.h"
#include "AtomDBUtils.h"
#include "InMemoryDB.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"

using namespace query_engine;
using namespace atomdb;
using namespace atoms;
using namespace std;
using das_test::init_test_system_parameters_singleton;

TEST(AtomDBTest, handle_to_metta) {
    // AtomDBSingleton::init(test_atomdb_json_config());
    AtomDBSingleton::init(test_atomdb_json_config("redismongodb", "base_query_proxy_test_"));
    init_test_system_parameters_singleton();

    auto db = AtomDBSingleton::get_instance();

    auto A = new Node("Symbol", "A");
    auto B = new Node("Symbol", "B");
    auto C = new Node("Symbol", "C");
    auto D = new Node("Symbol", "D");

    db->add_nodes({A, B, C, D});

    // (A)
    auto L1 = new Link("Expression", {A->handle()}, true);
    db->add_link(L1);
    EXPECT_EQ(AtomDBUtils::handle_to_metta(L1->handle()), "(A)");

    // (A B)
    auto L2 = new Link("Expression", {A->handle(), B->handle()}, true);
    db->add_link(L2);
    EXPECT_EQ(AtomDBUtils::handle_to_metta(L2->handle()), "(A B)");

    // ((A) (A B))
    auto L3 = new Link("Expression", {L1->handle(), L2->handle()}, true);
    db->add_link(L3);
    EXPECT_EQ(AtomDBUtils::handle_to_metta(L3->handle()), "((A) (A B))");

    // (((A) (A B)) (A B) C)
    auto L4 = new Link("Expression", {L3->handle(), L2->handle(), C->handle()}, true);
    db->add_link(L4);
    EXPECT_EQ(AtomDBUtils::handle_to_metta(L4->handle()), "(((A) (A B)) (A B) C)");

    // (D (((A) (A B)) (A B) C) ((A) (A B)) (((A) (A B)) (A B) C))
    auto L5 = new Link("Expression", {D->handle(), L4->handle(), L3->handle(), L4->handle()}, true);
    db->add_link(L5);
    EXPECT_EQ(AtomDBUtils::handle_to_metta(L5->handle()),
              "(D (((A) (A B)) (A B) C) ((A) (A B)) (((A) (A B)) (A B) C))");

    QueryAnswer answer(L5->handle(), 0);
    PatternMatchingQueryProxy proxy;
    proxy.populate_metta_mapping(&answer);

    EXPECT_EQ(answer.metta_expression[A->handle()], "A");
    EXPECT_EQ(answer.metta_expression[B->handle()], "B");
    EXPECT_EQ(answer.metta_expression[C->handle()], "C");
    EXPECT_EQ(answer.metta_expression[D->handle()], "D");
    EXPECT_EQ(answer.metta_expression[L1->handle()], "(A)");
    EXPECT_EQ(answer.metta_expression[L2->handle()], "(A B)");
    EXPECT_EQ(answer.metta_expression[L3->handle()], "((A) (A B))");
    EXPECT_EQ(answer.metta_expression[L4->handle()], "(((A) (A B)) (A B) C)");
    EXPECT_EQ(answer.metta_expression[L5->handle()],
              "(D (((A) (A B)) (A B) C) ((A) (A B)) (((A) (A B)) (A B) C))");

    db->delete_links({L1->handle(), L2->handle(), L3->handle(), L4->handle(), L5->handle()}, true);
}
