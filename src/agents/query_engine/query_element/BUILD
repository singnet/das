load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "query_element_lib",
    includes = ["."],
    deps = [
        ":and",
        ":importance_list",
        ":iterator",
        ":link_template",
        ":link_template2",
        ":operator",
        ":or",
        ":query_element",
        ":query_element_registry",
        ":sink",
        ":source",
        ":terminal",
        ":unique_assignment_filter",
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
    srcs = ["Terminal.cc"],
    hdrs = ["Terminal.h"],
    deps = [
        ":query_element",
        "//atomdb:atomdb_singleton",
        "//commons/atoms:atoms_lib",
        "//hasher:hasher_lib",
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
        ":importance_list",
        ":iterator",
        ":or",
        ":query_element_registry",
        ":source",
        ":terminal",
        "//agents/query_engine:query_node",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//hasher:hasher_lib",
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
    srcs = ["QueryElementRegistry.cc"],
    hdrs = ["QueryElementRegistry.h"],
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
        "//attention_broker:attention_broker_lib",
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
