#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/trampoline.h>

#include <type_traits>

#include "query_engine/DASNode.h"
#include "query_engine/HandlesAnswer.h"
#include "query_engine/QueryAnswer.h"
#include "query_engine/query_element/RemoteIterator.h"

namespace nb = nanobind;
using namespace nb::literals;

using namespace std;
using namespace query_engine;

NB_MODULE(hyperon_das_query_engine_ext, m) {
    // QueryAnswer.h bindings
    nb::class_<QueryAnswer>(m, "QueryAnswer")
        .def("tokenize", &QueryAnswer::tokenize)
        .def("untokenize", &QueryAnswer::untokenize, "tokens"_a)
        .def("to_string", &QueryAnswer::to_string);

    // HandlesAnswer.h
    nb::class_<HandlesAnswer, QueryAnswer>(m, "HandlesAnswer")
        .def(nb::init<>())
        .def(nb::init<double>(), "importance"_a)
        .def(
            "__init__",
            [](HandlesAnswer& self, const string& handle, double importance) {
                new (&self) HandlesAnswer(handle.c_str(), importance);
            },
            "handle"_a,
            "importance"_a)
        .def_prop_ro("handles",
                     [](const HandlesAnswer& self) -> const vector<string> {
                         vector<string> handles;
                         for (size_t i = 0; i < self.handles_size; i++) {
                             handles.emplace_back(self.handles[i]);
                         }
                         return handles;
                     })
        .def_ro("handles_size", &HandlesAnswer::handles_size)
        .def_ro("importance", &HandlesAnswer::importance)
        .def(
            "add_handle",
            [](HandlesAnswer& self, const string& handle) { self.add_handle(handle.c_str()); },
            "handle"_a)
        .def(
            "merge",
            [](HandlesAnswer& self, const HandlesAnswer& other, bool merge_handles) -> bool {
                return self.merge((HandlesAnswer*) &other, merge_handles);
            },
            "other"_a,
            "merge_handles"_a)
        .def_static(
            "copy",
            [](const HandlesAnswer& other) -> shared_ptr<HandlesAnswer> {
                return shared_ptr<HandlesAnswer>(HandlesAnswer::copy((HandlesAnswer*) &other));
            },
            // C++ implementation named it `base` instead of `other`, but `other` is more idiomatic
            "other"_a);

    // RemoteIterator.h
    nb::class_<RemoteIterator<HandlesAnswer>>(m, "RemoteIterator")
        .def(nb::init<const string&>(), "local_id"_a)
        .def_ro("is_terminal", &RemoteIterator<HandlesAnswer>::is_terminal)
        .def("graceful_shutdown", &RemoteIterator<HandlesAnswer>::graceful_shutdown)
        .def("setup_buffers", &RemoteIterator<HandlesAnswer>::setup_buffers)
        .def("finished", &RemoteIterator<HandlesAnswer>::finished)
        .def("pop", [](RemoteIterator<HandlesAnswer>& self) -> shared_ptr<HandlesAnswer> {
            return shared_ptr<HandlesAnswer>((HandlesAnswer*) self.pop());
        });

    // DASNode.h
    nb::class_<DASNode>(m, "DASNode")
        .def(nb::init<const string&>(), "node_id"_a)
        .def(nb::init<const string&, const string&>(), "node_id"_a, "server_id"_a)
        .def_ro_static("PATTERN_MATCHING_QUERY", &DASNode::PATTERN_MATCHING_QUERY)
        .def_ro_static("COUNTING_QUERY", &DASNode::COUNTING_QUERY)
        .def(
            "pattern_matcher_query",
            [](DASNode& self,
               const vector<string>& tokens,
               const string& context,
               bool update_attention_broker) -> shared_ptr<RemoteIterator<HandlesAnswer>> {
                return shared_ptr<RemoteIterator<HandlesAnswer>>(
                    self.pattern_matcher_query(tokens, context, update_attention_broker));
            },
            "tokens"_a,
            "context"_a = "",
            "update_attention_broker"_a = false)
        .def("count_query",
             &DASNode::count_query,
             "tokens"_a,
             "context"_a = "",
             "update_attention_broker"_a = false)
        .def("next_query_id", &DASNode::next_query_id)
        .def("message_factory", &DASNode::message_factory, "command"_a, "args"_a)

        // inherited from DistributedAlgorithmNode
        .def("join_network", &DASNode::join_network)
        .def("is_leader", &DASNode::is_leader)
        .def("leader_id", &DASNode::leader_id)
        .def("has_leader", &DASNode::has_leader)
        .def("add_peer", &DASNode::add_peer, "peer_id"_a)
        .def("node_id", &DASNode::node_id)
        .def("broadcast", &DASNode::broadcast, "command"_a, "args"_a)
        .def("send", &DASNode::send, "command"_a, "args"_a, "recipient"_a)
        .def("node_joined_network", &DASNode::node_joined_network, "node_id"_a)
        .def("cast_leadership_vote", &DASNode::cast_leadership_vote);
}
