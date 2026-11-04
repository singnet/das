#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Assignment.h"
#include "AtomDBInitializer.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"
#include "RedisMongoDBAPITypes.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"

using das_test::init_test_system_parameters_singleton;
using namespace atomdb::atomdb_api_types;
using namespace atomdb;
using namespace atoms;
using namespace commons;
using namespace std;

class TestDecoder : public HandleDecoder {
   public:
    map<string, shared_ptr<Atom>> atoms;
    shared_ptr<Atom> decode_atom(const string& handle, shared_ptr<Keychain> keychain = nullptr) {
        return this->atoms[handle];
    }
    string decode_node_name(const string& handle, shared_ptr<Keychain> keychain = nullptr) {
        auto atom = decode_atom(handle, keychain);
        if (atom != nullptr) {
            auto node = dynamic_pointer_cast<Node>(atom);
            if (node != nullptr) {
                return node->name;
            }
        }
        return "NO_NAME";
    }
    vector<string> decode_link_targets(const string& handle, shared_ptr<Keychain> keychain = nullptr) {
        auto atom = decode_atom(handle, keychain);
        if (atom != nullptr) {
            auto link = dynamic_pointer_cast<Link>(atom);
            if (link != nullptr) {
                return link->targets;
            }
        }
        return vector<string>({});
    }
    shared_ptr<Atom> add_atom(shared_ptr<Atom> atom) {
        this->atoms[atom->handle()] = atom;
        return atom;
    }
};

namespace {

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
    TestDecoder decoder;
    auto inheritance = make_shared<Node>("Symbol", "Inheritance");
    auto human = make_shared<Node>("Symbol", "\"human\"");
    auto monkey = make_shared<Node>("Symbol", "\"monkey\"");
    auto mammal = make_shared<Node>("Symbol", "\"mammal\"");
    decoder.add_atom(inheritance);
    decoder.add_atom(human);
    decoder.add_atom(monkey);
    decoder.add_atom(mammal);

    auto non_matching = make_shared<Link>(
        "Expression", vector<string>{inheritance->handle(), human->handle(), monkey->handle()});
    auto matching = make_shared<Link>(
        "Expression", vector<string>{inheritance->handle(), human->handle(), mammal->handle()});
    auto second_matching = make_shared<Link>(
        "Expression", vector<string>{inheritance->handle(), monkey->handle(), mammal->handle()});
    decoder.add_atom(non_matching);
    decoder.add_atom(matching);
    decoder.add_atom(second_matching);

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
    auto it = handle_set->get_iterator(&decoder);
    char* handle = it->next();
    ASSERT_FALSE(handle == NULL);
    EXPECT_EQ(string(handle), matching->handle());
    handle = it->next();
    ASSERT_FALSE(handle == NULL);
    EXPECT_EQ(string(handle), second_matching->handle());
    EXPECT_TRUE(it->next() == NULL);

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
    AtomDBInitializer::init(test_atomdb_json_config());
    return RUN_ALL_TESTS();
}
