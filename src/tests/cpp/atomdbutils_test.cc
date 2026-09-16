#include "AtomDBUtils.h"

#include <gtest/gtest.h>

#include "AtomDBSingleton.h"
#include "InMemoryDB.h"
#include "TestAtomDBJsonConfig.h"

using namespace atomdb;
using namespace atoms;
using namespace std;

TEST(AtomDBTest, reachable_terminal_set) {
    AtomDBSingleton::init(test_atomdb_json_config("redismongodb", "atomdbutils_test_"));
    auto db = AtomDBSingleton::get_instance();

    auto A = new Node("Symbol", "A");
    auto B = new Node("Symbol", "B");
    auto C = new Node("Symbol", "C");
    auto D = new Node("Symbol", "D");
    auto E = new Node("Symbol", "E");
    auto F = new Node("Symbol", "F");
    auto G = new Node("Symbol", "G");
    auto H = new Node("Symbol", "H");
    auto I = new Node("Symbol", "I");
    auto J = new Node("Symbol", "J");
    auto K = new Node("Symbol", "K");
    auto NOT_ADDED = new Node("Symbol", "NOT_ADDED");
    db->add_nodes({A, B, C, D, E, F, G, H, I, J, K});

    auto L6 = new Link("Expression", {I->handle(), J->handle(), K->handle()}, true);
    db->add_link(L6);
    auto L5 = new Link("Expression", {C->handle(), D->handle()}, true);
    db->add_link(L5);
    auto L4 = new Link("Expression", {L5->handle(), E->handle()}, true);
    db->add_link(L4);
    auto L3 = new Link("Expression", {L4->handle(), F->handle()}, true);
    db->add_link(L3);
    auto L2 = new Link("Expression", {G->handle(), L6->handle(), H->handle()}, true);
    db->add_link(L2);
    auto L1 = new Link("Expression", {A->handle(), B->handle(), L3->handle()}, true);
    db->add_link(L1);
    auto L0 = new Link("Expression", {L1->handle(), L2->handle()}, true);
    db->add_link(L0);

    string a = A->handle();
    string b = B->handle();
    string c = C->handle();
    string d = D->handle();
    string e = E->handle();
    string f = F->handle();
    string g = G->handle();
    string h = H->handle();
    string i = I->handle();
    string j = J->handle();
    string k = K->handle();

    set<string> handles;

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, NOT_ADDED->handle());
    EXPECT_EQ(handles.size(), 0);

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, a);
    EXPECT_EQ(handles, set({a}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L1->handle());
    EXPECT_EQ(handles, set({a, b, c, d, e, f}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L2->handle());
    EXPECT_EQ(handles, set({g, h, i, j, k}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L3->handle());
    EXPECT_EQ(handles, set({c, d, e, f}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L4->handle());
    EXPECT_EQ(handles, set({c, d, e}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L5->handle());
    EXPECT_EQ(handles, set({c, d}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L6->handle());
    EXPECT_EQ(handles, set({i, j, k}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L0->handle());
    EXPECT_EQ(handles, set({a, b, c, d, e, f, g, h, i, j, k}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L6->handle(), true);
    EXPECT_EQ(handles, set({j, k}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L2->handle(), true);
    EXPECT_EQ(handles, set({h, j, k}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L4->handle(), true);
    EXPECT_EQ(handles, set({d, e}));

    handles.clear();
    AtomDBUtils::reachable_terminal_set(handles, L1->handle(), true);
    EXPECT_EQ(handles, set({b, d, e, f}));

    db->delete_links({L0->handle(),
                      L1->handle(),
                      L2->handle(),
                      L3->handle(),
                      L4->handle(),
                      L5->handle(),
                      L6->handle()},
                     true);
}

TEST(AtomDBTest, handle_to_metta) {
    auto db = AtomDBSingleton::get_instance();

    auto A = new Node("Symbol", "A");
    auto B = new Node("Symbol", "B");
    auto C = new Node("Symbol", "C");
    auto D = new Node("Symbol", "D");

    db->add_nodes({A, B, C, D});

    // A
    EXPECT_EQ(AtomDBUtils::handle_to_metta(A->handle()), "A");

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

    db->delete_links({L1->handle(), L2->handle(), L3->handle(), L4->handle(), L5->handle()}, true);
}
