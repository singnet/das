#include <gtest/gtest.h>

#include "atom_space/AtomSpaceTypes.h"

using namespace std;
using namespace atomspace;

// Helper to free char* returned by compute_handle
struct HandleDeleter {
    void operator()(char* ptr) const { free(ptr); }
};

TEST(AtomTest, AtomTypeValidation) {
    EXPECT_NO_THROW(Atom("Type"));
    EXPECT_THROW(Atom(""), runtime_error);
}

TEST(NodeTest, NodeValidationAndToString) {
    EXPECT_NO_THROW(Node("Type", "Name"));
    EXPECT_THROW(Node("Type", ""), runtime_error);
    EXPECT_THROW(Node("", "Name"), runtime_error);

    Node n("Type", "Name");
    string s = n.to_string();
    EXPECT_EQ(s, "Node(type: 'Type', name: 'Name', custom_attributes: {})");
}

TEST(NodeTest, NodeComputeHandle) {
    unique_ptr<char, HandleDeleter> handle(Node::compute_handle("Type", "Name"));
    ASSERT_NE(handle.get(), nullptr);
}

TEST(LinkTest, LinkValidation) {
    Node n1("Type1", "Name1");
    Node n2("Type2", "Name2");
    vector<const Atom*> targets = {&n1, &n2};
    EXPECT_NO_THROW(Link("LinkType", targets));
    vector<const Atom*> empty_targets;
    EXPECT_THROW(Link("LinkType", empty_targets), runtime_error);
    EXPECT_THROW(Link("", targets), runtime_error);
}

TEST(LinkTest, LinkToString) {
    Node n1("Type1", "Name1");
    Node n2("Type2", "Name2");
    Link l("LinkType", {&n1, &n2});
    string s = l.to_string();
    // clang-format off
    EXPECT_EQ(
        s,
        "Link("
            "type: 'LinkType', "
            "targets: ["
                "Node(type: 'Type1', name: 'Name1', custom_attributes: {}), "
                "Node(type: 'Type2', name: 'Name2', custom_attributes: {})"
            "], "
            "custom_attributes: {}"
        ")"
    );
    // clang-format on
}

TEST(LinkTest, LinkTargetsToHandles) {
    Node n1("Type1", "Name1");
    Node n2("Type2", "Name2");
    vector<const Atom*> targets = {&n1, &n2};
    unique_ptr<char*[], TargetHandlesDeleter> handles(Link::targets_to_handles(targets),
                                                      TargetHandlesDeleter(targets.size()));
    ASSERT_NE(handles, nullptr);
    for (size_t i = 0; i < targets.size(); ++i) {
        ASSERT_NE(handles[i], nullptr);
    }
}

TEST(LinkTest, LinkComputeHandleFromAtoms) {
    Node n1("Type1", "Name1");
    Node n2("Type2", "Name2");
    vector<const Atom*> targets = {&n1, &n2};
    unique_ptr<char, HandleDeleter> handle(Link::compute_handle("LinkType", targets));
    ASSERT_NE(handle.get(), nullptr);
}

TEST(LinkTest, LinkComputeHandleFromStrings) {
    vector<string> handles = {"handle1", "handle2"};
    unique_ptr<char, HandleDeleter> handle(Link::compute_handle("LinkType", handles));
    ASSERT_NE(handle.get(), nullptr);
}

TEST(LinkTest, LinkComputeHandleFromRaw) {
    vector<string> handles = {"handle1", "handle2"};
    char* type_hash = named_type_hash((char*) "LinkType");
    char** targets_handles = new char*[handles.size()];
    for (size_t i = 0; i < handles.size(); ++i) targets_handles[i] = (char*) handles[i].c_str();
    unique_ptr<char, HandleDeleter> handle(
        Link::compute_handle("LinkType", targets_handles, handles.size()));
    ASSERT_NE(handle.get(), nullptr);
    delete[] targets_handles;
    free(type_hash);
}

TEST(NodeTest, NodeWithCustomAttributes) {
    CustomAttributesMap attrs;
    attrs["foo"] = std::string("bar");
    attrs["num"] = 42L;
    Node n("Type", "Name", attrs);
    EXPECT_EQ(n.custom_attributes.get<std::string>("foo") != nullptr, true);
    EXPECT_EQ(n.custom_attributes.get<long>("num") != nullptr, true);
    std::string s = n.to_string();
    EXPECT_EQ(s, "Node(type: 'Type', name: 'Name', custom_attributes: {foo: 'bar', num: 42})");
}

TEST(LinkTest, LinkWithCustomAttributes) {
    Node n1("Type1", "Name1");
    Node n2("Type2", "Name2");
    std::vector<const Atom*> targets = {&n1, &n2};
    Link l("LinkType", targets, {{"flag", true}, {"count", 10}});
    EXPECT_EQ(l.custom_attributes.get<bool>("flag") != nullptr, true);
    std::string s = l.to_string();
    // clang-format off
    EXPECT_EQ(
        s,
        "Link("
            "type: 'LinkType', "
            "targets: ["
                "Node(type: 'Type1', name: 'Name1', custom_attributes: {}), "
                "Node(type: 'Type2', name: 'Name2', custom_attributes: {})"
            "], "
            "custom_attributes: {count: 10, flag: true}"
        ")"
    );
    // clang-format on
}