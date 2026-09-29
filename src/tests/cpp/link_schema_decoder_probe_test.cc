#include <gtest/gtest.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Assignment.h"
#include "HandleDecoder.h"
#include "InMemoryDB.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"

using namespace std;
using namespace atoms;
using namespace atomdb;
using namespace commons;

namespace {

struct Fixture {
    // Nodes
    shared_ptr<Node> similarity = make_shared<Node>("Symbol", "Similarity");
    shared_ptr<Node> evaluation = make_shared<Node>("Symbol", "Evaluation");
    shared_ptr<Node> predicate = make_shared<Node>("Symbol", "Predicate");
    shared_ptr<Node> is_animal = make_shared<Node>("Symbol", "\"is_animal\"");
    shared_ptr<Node> human = make_shared<Node>("Symbol", "\"human\"");
    shared_ptr<Node> monkey = make_shared<Node>("Symbol", "\"monkey\"");

    // Links
    // (Similarity "human" "monkey")
    shared_ptr<Link> similarity_human_monkey = make_shared<Link>(
        "Expression", vector<string>{similarity->handle(), human->handle(), monkey->handle()});
    // (Predicate "is_animal")
    shared_ptr<Link> predicate_is_animal =
        make_shared<Link>("Expression", vector<string>{predicate->handle(), is_animal->handle()});
    // (Evaluation (Predicate "is_animal") "human")
    shared_ptr<Link> evaluation_is_animal_human = make_shared<Link>(
        "Expression",
        vector<string>{evaluation->handle(), predicate_is_animal->handle(), human->handle()});

    // Schemas
    // Flat: (Similarity "human" $V)  -- targets are NODE / VARIABLE only
    LinkSchema flat_schema{vector<string>{"LINK_TEMPLATE",
                                          "Expression",
                                          "3",
                                          "NODE",
                                          "Symbol",
                                          "Similarity",
                                          "NODE",
                                          "Symbol",
                                          "\"human\"",
                                          "VARIABLE",
                                          "V"}};
    // Nested grounded LINK: (Evaluation (Predicate "is_animal") $C)
    LinkSchema nested_link_schema{vector<string>{"LINK_TEMPLATE",
                                                 "Expression",
                                                 "3",
                                                 "NODE",
                                                 "Symbol",
                                                 "Evaluation",
                                                 "LINK",
                                                 "Expression",
                                                 "2",
                                                 "NODE",
                                                 "Symbol",
                                                 "Predicate",
                                                 "NODE",
                                                 "Symbol",
                                                 "\"is_animal\"",
                                                 "VARIABLE",
                                                 "C"}};
    // Nested LINK_TEMPLATE: (Evaluation (Predicate $P) $C)
    LinkSchema nested_template_schema{vector<string>{"LINK_TEMPLATE",
                                                     "Expression",
                                                     "3",
                                                     "NODE",
                                                     "Symbol",
                                                     "Evaluation",
                                                     "LINK_TEMPLATE",
                                                     "Expression",
                                                     "2",
                                                     "NODE",
                                                     "Symbol",
                                                     "Predicate",
                                                     "VARIABLE",
                                                     "P",
                                                     "VARIABLE",
                                                     "C"}};
};

}  // namespace

TEST(LinkSchemaDecoderProbe, Step3_InMemoryDBMissingTargets) {
    Fixture f;

    auto db = make_shared<InMemoryDB>();

    // (Similarity "human" "monkey")
    db->add_link(f.similarity_human_monkey.get());
    // (Evaluation (Predicate "is_animal") "human")
    db->add_link(f.evaluation_is_animal_human.get());

    // Case 1: Flat schema matches
    // similarity_human_monkey: (Similarity "human" "monkey")
    // flat_schema:             (Similarity "human" $V) -> "monkey" (handle)
    Assignment a;
    EXPECT_TRUE(f.flat_schema.match(*f.similarity_human_monkey, a, *db));
    EXPECT_EQ(a.get("V"), f.monkey->handle());

    // Case 2: Nested link schema DOES NOT match
    // evaluation_is_animal_human:  (Evaluation (Predicate "is_animal") "human")
    // nested_link_schema:          (Evaluation (Predicate "is_animal") $C) -> False [should be "human" (handle)]
    Assignment b;
    bool nested_ok = false;
    EXPECT_NO_THROW(nested_ok = f.nested_link_schema.match(*f.evaluation_is_animal_human, b, *db));
    EXPECT_FALSE(nested_ok);

    // Case 3: Nested template schema DOES NOT match
    // evaluation_is_animal_human:  (Evaluation (Predicate "is_animal") "human")
    // nested_template_schema:      (Evaluation (Predicate $P) $C) -> False [should be "is_animal" (handle) and "human" (handle)]
    Assignment c;
    bool template_ok = false;
    EXPECT_NO_THROW(template_ok = f.nested_template_schema.match(*f.evaluation_is_animal_human, c, *db));
    EXPECT_FALSE(template_ok);

    // Case 4: Toplevel-by-handle for a link that is stored still decodes fine ...
    // similarity_human_monkey: (Similarity "human" "monkey")
    // flat_schema:             (Similarity "human" $V) -> "monkey" (handle)
    Assignment d;
    EXPECT_TRUE(f.flat_schema.match(f.similarity_human_monkey->handle(), d, *db));
    EXPECT_EQ(d.get("V"), f.monkey->handle());

    // Case 5: For a handle that is not stored, decoding returns nullptr -> no match.
    // human:                   "human" (handle)
    // flat_schema:             (Similarity "human" $V) -> no match [should be "human" (handle)]
    Assignment e;
    EXPECT_FALSE(f.flat_schema.match(f.human->handle(), e, *db));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
