#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
//TODO: remove unused imports
// #include <nanobind/stl/vector.h>
// #include <nanobind/stl/shared_ptr.h>
#include <nanobind/trampoline.h>

#include "query_engine/DASNode.h"
// #include "query_engine/HandlesAnswer.h"
#include "query_engine/QueryAnswer.h"

// #include "query_engine/query_element/RemoteIterator.h"

namespace nb = nanobind;
using namespace nb::literals;

using namespace std;
using namespace query_engine;

//TODO: make sure we need to override QueryAnswer methods
// if so, neeed to change nb:class
//
// class QueryAnswerTrampoline : public QueryAnswer {
//   NB_TRAMPOLINE(QueryAnswer, 3);
//   const string& tokenize() override {
//     NB_OVERRIDE_PURE(tokenize);
//   }
//   void untokenize(const string& tokens) override {
//     NB_OVERRIDE_PURE(untokenize, tokens);
//   }
//   string to_string() override {
//     NB_OVERRIDE_PURE(to_string);
//   }
// };


NB_MODULE(hyperon_das_query_engine_ext, m) {
  //QueryAnswer.h bindings
  nb::class_<QueryAnswer>(m, "QueryAnswer")
    .def("tokenize", &QueryAnswer::tokenize)
    .def("untokenize", &QueryAnswer::untokenize, "tokens"_a)
    .def("to_string", &QueryAnswer::to_string);

  //HandlesAnswer.h

  //DASNode.h
  nb::class_<DASNode>(m, "DASNode")
    .def(nb::init<string>(), "node_id"_a)
    .def(nb::init<string, string>(), "node_id"_a, "server_id"_a)
    .def_ro_static("PATTERN_MATCHING_QUERY", &DASNode::PATTERN_MATCHING_QUERY)
    .def_ro_static("COUNTING_QUERY", &DASNode::COUNTING_QUERY)
    .def("pattern_matcher_query", &DASNode::pattern_matcher_query,
         "tokens"_a,
         "context"_a = "",
         "update_attention_broker"_a = false)
    .def("count_query", &DASNode::count_query,
         "tokens"_a,
         "context"_a = "",
         "update_attention_broker"_a = false)
    .def("next_query_id", &DASNode::next_query_id)
    .def("message_factory", &DASNode::message_factory,
         "command"_a,
         "args"_a)

  // inherited from DistributedAlgorithmNode
    .def("join_network", &DASNode::join_network)
    .def("is_leader", &DASNode::is_leader)
    .def("leader_id", &DASNode::leader_id)
    .def("has_leader", &DASNode::has_leader)
    .def("add_peer", &DASNode::add_peer, "peer_id"_a)
    .def("node_id", &DASNode::node_id)
    .def("broadcast", &DASNode::broadcast, "command"_a, "args"_a)
    .def("send", &DASNode::send, "command"_a, "args"_a, "recipient"_a)
    .def("node_joined_network", &DASNode::node_joined_network,
          "node_id"_a)
    .def("cast_leadership_vote", &DASNode::cast_leadership_vote);

  //RemoteIterator.h
}

//TODO: Expose DASNode
