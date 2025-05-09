package(default_visibility = ["//visibility:public"])

cc_library(
  name = "query_element_lib",
  includes = ["."],
  deps = [
    ":operator",
    ":and",
    ":or",
    ":terminal",
    ":iterator",
    ":sink",
    ":query_element",
    ":link_template",
    ":link_template2",
    ":source",
    ":unique_assignment_filter",
    ":query_element_registry",
    ":importance_list",
  ],
)

cc_library(
  name = "operator",
  hdrs = ["Operator.h"],
  deps = [
    ":query_element",
    "//agents/query_engine:query_answer",
  ],
)

cc_library(
  name = "and",
  hdrs = ["And.h"],
  deps = [
    ":operator",
  ],
)

cc_library(
  name = "or",
  hdrs = ["Or.h"],
  deps = [
    ":operator",
  ],
)

cc_library(
  name = "terminal",
  hdrs = ["Terminal.h"],
  deps = [
    "//atomdb:atomdb_singleton",
    "//hasher:hasher_lib"

  ],
)

cc_library(
  name = "iterator",
  srcs = ["Iterator.cc"],
  hdrs = ["Iterator.h"],
  deps = [
    ":sink",
    "//agents/query_engine:query_answer",
  ],
)

cc_library(
  name = "sink",
  srcs = ["Sink.cc"],
  hdrs = ["Sink.h"],
  deps = [
    ":query_element",
  ],
)

cc_library(
  name = "query_element",
  srcs = ["QueryElement.cc"],
  hdrs = ["QueryElement.h"],
  deps = [
    "//agents/query_engine:query_node",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "link_template",
  hdrs = ["LinkTemplate.h"],
  deps = [
    ":and",
    ":or",
    ":importance_list",
    ":iterator",
    ":query_element_registry",
    ":terminal",
    ":source",
    "//hasher:hasher_lib",
    "//atomdb:atomdb_api_types",
    "//atomdb:atomdb_singleton",
    "//agents/query_engine:query_node",
    "//attention_broker:attention_broker_server_lib",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "link_template2",
  hdrs = ["LinkTemplate2.h"],
  deps = [
    ":link_template",
  ],
)

cc_library(
  name = "query_element_registry",
  hdrs = ["QueryElementRegistry.h"],
  srcs = ["QueryElementRegistry.cc"],
  deps = [
    ":query_element",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "importance_list",
  hdrs = ["ImportanceList.h"],
  deps = [
    ":query_element",
    "//attention_broker:attention_broker_server_lib",
  ],
)

cc_library(
  name = "unique_assignment_filter",
  srcs = ["UniqueAssignmentFilter.cc"],
  hdrs = ["UniqueAssignmentFilter.h"],
  deps = [
    ":query_element",
  ],
)

cc_library(
  name = "source",
  srcs = ["Source.cc"],
  hdrs = ["Source.h"],
  deps = [
    ":query_element",
  ],
)
