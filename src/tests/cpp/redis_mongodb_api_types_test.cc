#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Assignment.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"
#include "RedisMongoDBAPITypes.h"

using namespace atomdb::atomdb_api_types;
using namespace atoms;
using namespace commons;
using namespace std;

namespace {

class MapDecoder : public HandleDecoder {
   public:
    shared_ptr<Atom> get_atom(const string& handle) override {
        auto it = atoms.find(handle);
        return it == atoms.end() ? nullptr : it->second;
    }

    void add(const shared_ptr<Atom>& atom) { atoms[atom->handle()] = atom; }

   private:
    map<string, shared_ptr<Atom>> atoms;
};

redisReply* make_array_reply(const vector<string>& values) {
    auto* reply = static_cast<redisReply*>(calloc(1, sizeof(redisReply)));
    reply->type = REDIS_REPLY_ARRAY;
    reply->elements = values.size();
    reply->element = static_cast<redisReply**>(calloc(values.size(), sizeof(redisReply*)));
    for (size_t i = 0; i < values.size(); ++i) {
        reply->element[i] = static_cast<redisReply*>(calloc(1, sizeof(redisReply)));
        reply->element[i]->type = REDIS_REPLY_STRING;
        reply->element[i]->len = values[i].size();
        reply->element[i]->str = strdup(values[i].c_str());
    }
    return reply;
}

}  // namespace

TEST(HandleSetRedisIteratorTest, FiltersCandidatesAndStoresAssignmentForEachMatch) {
    MapDecoder decoder;
    auto inheritance = make_shared<Node>("Symbol", "Inheritance");
    auto human = make_shared<Node>("Symbol", "\"human\"");
    auto monkey = make_shared<Node>("Symbol", "\"monkey\"");
    auto mammal = make_shared<Node>("Symbol", "\"mammal\"");
    decoder.add(inheritance);
    decoder.add(human);
    decoder.add(monkey);
    decoder.add(mammal);

    auto non_matching = make_shared<Link>(
        "Expression", vector<string>{inheritance->handle(), human->handle(), monkey->handle()});
    auto matching = make_shared<Link>(
        "Expression", vector<string>{inheritance->handle(), human->handle(), mammal->handle()});
    auto second_matching = make_shared<Link>(
        "Expression", vector<string>{inheritance->handle(), monkey->handle(), mammal->handle()});
    decoder.add(non_matching);
    decoder.add(matching);
    decoder.add(second_matching);

    auto handle_set = make_shared<HandleSetRedis>(
        make_array_reply({non_matching->handle(), matching->handle(), second_matching->handle()}));
    handle_set->link_schema = make_shared<LinkSchema>(vector<string>{"LINK_TEMPLATE",
                                                                     "Expression",
                                                                     "3",
                                                                     "NODE",
                                                                     "Symbol",
                                                                     "Inheritance",
                                                                     "VARIABLE",
                                                                     "x",
                                                                     "NODE",
                                                                     "Symbol",
                                                                     "\"mammal\""});
    handle_set->decoder = &decoder;

    auto it = handle_set->get_iterator();
    char* handle = it->next();
    ASSERT_NE(handle, nullptr);
    EXPECT_EQ(string(handle), matching->handle());
    handle = it->next();
    ASSERT_NE(handle, nullptr);
    EXPECT_EQ(string(handle), second_matching->handle());
    EXPECT_EQ(it->next(), nullptr);

    auto first_assignment = handle_set->get_assignments_by_handle(matching->handle());
    EXPECT_EQ(first_assignment.variable_count(), 1u);
    EXPECT_EQ(first_assignment.get("x"), human->handle());
    auto second_assignment = handle_set->get_assignments_by_handle(second_matching->handle());
    EXPECT_EQ(second_assignment.variable_count(), 1u);
    EXPECT_EQ(second_assignment.get("x"), monkey->handle());
    EXPECT_EQ(handle_set->get_assignments_by_handle(non_matching->handle()).variable_count(), 0u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
