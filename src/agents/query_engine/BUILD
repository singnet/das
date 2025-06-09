load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "query_engine_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    deps = [
        ":pattern_matching_query_processor",
        ":pattern_matching_query_proxy",
        ":query_answer",
        ":query_node",
        "//agents/query_engine/query_element:query_element_lib",
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//distributed_algorithm_node:star_node",
        "//hasher:hasher_lib",
        "@mbedtls",
    ],
)

cc_library(
    name = "query_answer",
    srcs = ["QueryAnswer.cc"],
    hdrs = ["QueryAnswer.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "query_node",
    srcs = ["QueryNode.cc"],
    hdrs = ["QueryNode.h"],
    includes = ["."],
    deps = [
        ":query_answer",
        "//commons:commons_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "pattern_matching_query_proxy",
    srcs = ["PatternMatchingQueryProxy.cc"],
    hdrs = ["PatternMatchingQueryProxy.h"],
    deps = [
        ":query_answer",
        "//distributed_algorithm_node",
        "//service_bus:bus_command_proxy",
    ],
)

cc_library(
    name = "pattern_matching_query_processor",
    srcs = ["PatternMatchingQueryProcessor.cc"],
    hdrs = ["PatternMatchingQueryProcessor.h"],
    deps = [
        ":pattern_matching_query_proxy",
        "//agents/query_engine/query_element:query_element_lib",
        "//service_bus:bus_command_processor",
    ],
)
