#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/trampoline.h>

#include <type_traits>

#include "distributed_algorithm_node/DistributedAlgorithmNode.h"
#include "distributed_algorithm_node/LeadershipBroker.h"
#include "distributed_algorithm_node/Message.h"
#include "distributed_algorithm_node/MessageBroker.h"
#include "distributed_algorithm_node/StarNode.h"
#include "agents/query_engine/QueryAnswer.h"

namespace nb = nanobind;
using namespace nb::literals;  // Enables use of literal "_a" for named arguments

using namespace std;
using namespace distributed_algorithm_node;
using namespace query_engine;

// Shared pointers aliases
using MessagePtr = shared_ptr<Message>;
using MessageFactoryPtr = shared_ptr<MessageFactory>;

// **************************** Python Trampolines ****************************
// Trampolines allow Python subclasses to override C++ methods.
// For more information:
// https://nanobind.readthedocs.io/en/latest/classes.html#overriding-virtual-functions-in-python

class MessageTrampoline : public Message {
    // Defines a trampoline for the BaseClass
    // The count (1) denotes the total number of virtual method slots that can be
    // overriden within Python.
    NB_TRAMPOLINE(Message, 1);
    void act(MessageFactoryPtr node) override {
        // Allows Python to override a pure virtual method
        NB_OVERRIDE_PURE(act, node);
    }
};

class MessageFactoryTrampoline : public MessageFactory {
   public:
    NB_TRAMPOLINE(MessageFactory, 1);
    MessagePtr message_factory(string& command, vector<string>& args) override {
        NB_OVERRIDE_PURE(message_factory, command, args);
    };
};

class DistributedAlgorithmNodeTrampoline : public DistributedAlgorithmNode {
   public:
    // Defines a trampoline for the BaseClass
    // Since we are overriding 3 methods in Python
    NB_TRAMPOLINE(DistributedAlgorithmNode, 3);
    MessagePtr message_factory(string& command, vector<string>& args) override {
        // Allows Python to override a non pure virtual method
        NB_OVERRIDE(message_factory, command, args);
    };
    void node_joined_network(const string& node_id) override {
        NB_OVERRIDE_PURE(node_joined_network, node_id);
    };
    string cast_leadership_vote() override { NB_OVERRIDE_PURE(cast_leadership_vote); };
};

class StarNodeTrampoline : public StarNode {
   public:
    NB_TRAMPOLINE(StarNode, 3);
    MessagePtr message_factory(string& command, vector<string>& args) override {
        // Allows Python to override a non pure virtual method
        NB_OVERRIDE(message_factory, command, args);
    };
    void node_joined_network(const string& node_id) override {
        NB_OVERRIDE(node_joined_network, node_id);
    };
    string cast_leadership_vote() override { NB_OVERRIDE(cast_leadership_vote); };
};
// ****************************************************************************

// Create the Python module 'hyperon_das_node_ext'
NB_MODULE(hyperon_das_node_ext, m) {
    // Message.h bindings
    nb::class_<Message, MessageTrampoline>(m, "Message")
        .def(nb::init<>())
        .def("act", &Message::act);  // Bind the act method (can be overriden in Python)

    nb::class_<MessageFactory, MessageFactoryTrampoline>(m, "MessageFactory")
        .def("message_factory", &MessageFactory::message_factory);  // Bind the message_factory method
                                                                    // (can be overriden in Python)

    // LeadershipBroker.h bindings
    // Binds the enum LeadershipBrokerType and all of it's values.
    // Needs to be updated whenever a new LeadershipBrokerType is added.
    nb::enum_<LeadershipBrokerType>(m, "LeadershipBrokerType")
        .value("SINGLE_MASTER_SERVER", LeadershipBrokerType::SINGLE_MASTER_SERVER);

    // MessageBroker.h bindings
    // Binds the enum MessageBrokerType and all of it's values.
    // Needs to be updated whenever a new MessageBrokerType is added.
    nb::enum_<MessageBrokerType>(m, "MessageBrokerType")
        .value("GRPC", MessageBrokerType::GRPC)
        .value("RAM", MessageBrokerType::RAM)
        .export_values();

    // DistributedAlgorithmNode.h bindings
    nb::class_<DistributedAlgorithmNode, MessageFactory, DistributedAlgorithmNodeTrampoline>(
        m, "DistributedAlgorithmNode")
        .def(nb::init<const string&, LeadershipBrokerType, MessageBrokerType>(),
             "node_id"_a,
             "leadership_algorithm"_a,
             "messaging_backend"_a)
        .def("join_network", &DistributedAlgorithmNode::join_network)
        .def("is_leader", &DistributedAlgorithmNode::is_leader)
        .def("leader_id", &DistributedAlgorithmNode::leader_id)
        .def("has_leader", &DistributedAlgorithmNode::has_leader)
        // Whenever we have a parameter that is a pointer or a reference, we need
        // to specify the name of the argument. Otherwise nanobind will add a
        // default arg0, arg1, etc.
        .def("add_peer", &DistributedAlgorithmNode::add_peer, "peer_id"_a)
        .def("node_id", &DistributedAlgorithmNode::node_id)
        .def("broadcast", &DistributedAlgorithmNode::broadcast, "command"_a, "args"_a)
        .def("send", &DistributedAlgorithmNode::send, "command"_a, "args"_a, "recipient"_a)
        .def("node_joined_network", &DistributedAlgorithmNode::node_joined_network, "node_id"_a)
        .def("cast_leadership_vote", &DistributedAlgorithmNode::cast_leadership_vote)
        .def("message_factory", &DistributedAlgorithmNode::message_factory);

    // StarNode.h bindings
    nb::class_<StarNode, DistributedAlgorithmNode, StarNodeTrampoline>(m, "StarNode")
        .def(nb::init<const string&, MessageBrokerType>(),
             "node_id"_a,
             "messaging_backend"_a = MessageBrokerType::GRPC)
        .def(nb::init<const string&, const string&, MessageBrokerType>(),
             "node_id"_a,
             "server_id"_a,
             "messaging_backend"_a = MessageBrokerType::GRPC);

    // TODO: Bindings after this comment belong to hyperon_das_query_engine_ext
    // This is an easy fix because DASNode is a child of StarNode, and
    // currently we could not figure out a way to make such inheritance without
    // duplicating the binding structure in hyperon_das_query_engine_ext.

    // QueryAnswer.h
    nb::class_<QueryAnswer>(m, "QueryAnswer")
        .def(nb::init<>())
        .def(nb::init<double>(), "importance"_a)
        .def(
            "__init__",
            [](QueryAnswer& self, const string& handle, double importance) {
                new (&self) QueryAnswer(handle.c_str(), importance);
            },
            "handle"_a,
            "importance"_a)
        .def_prop_ro("handles",
                     [](const QueryAnswer& self) -> const vector<string> {
                         vector<string> handles;
                         for (size_t i = 0; i < self.handles_size; i++) {
                             handles.emplace_back(self.handles[i]);
                         }
                         return handles;
                     })
        .def_ro("handles_size", &QueryAnswer::handles_size)
        .def_ro("importance", &QueryAnswer::importance)
        .def(
            "add_handle",
            [](QueryAnswer& self, const string& handle) { self.add_handle(handle.c_str()); },
            "handle"_a)
        .def(
            "merge",
            [](QueryAnswer& self, const QueryAnswer& other, bool merge_handles) -> bool {
                return self.merge((QueryAnswer*) &other, merge_handles);
            },
            "other"_a,
            "merge_handles"_a)
        .def_static(
            "copy",
            [](const QueryAnswer& other) -> shared_ptr<QueryAnswer> {
                return shared_ptr<QueryAnswer>(QueryAnswer::copy((QueryAnswer*) &other));
            },
            // C++ implementation named it `base` instead of `other`, but `other` is more idiomatic
            "other"_a);
}
