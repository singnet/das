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
        ":query_answer",
        ":query_node",
        ":pattern_matching_query_processor",
        ":pattern_matching_query_proxy",
        "//distributed_algorithm_node:star_node",
        "//query_engine/query_element:query_element_lib",
        "//attention_broker:attention_broker_server_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//commons:commons_lib",
        "@mbedtls//:mbedtls",
    ],
)

cc_library(
  name = "query_answer",
  srcs = ["QueryAnswer.cc"],
  hdrs = ["QueryAnswer.h"],
  includes = ["."],
  deps = [
    "//hasher:hasher_lib",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "query_node",
  srcs = ["QueryNode.cc"],
  hdrs = ["QueryNode.h"],
  includes = ["."],
  deps = [
    ":query_answer",
    "//hasher:hasher_lib",
    "//commons:commons_lib",
    "//distributed_algorithm_node:distributed_algorithm_node_lib",
  ],
)

cc_library(
  name = "pattern_matching_query_proxy",
  srcs = ["PatternMatchingQueryProxy.cc"],
  hdrs = ["PatternMatchingQueryProxy.h"],
  deps = [
    "//distributed_algorithm_node:distributed_algorithm_node",
    "//service_bus:bus_command_proxy",
    ":query_answer",
  ],
)

cc_library(
  name = "pattern_matching_query_processor",
  srcs = ["PatternMatchingQueryProcessor.cc"],
  hdrs = ["PatternMatchingQueryProcessor.h"],
  deps = [
    "//service_bus:bus_command_processor",
    "//query_engine/query_element:query_element_lib",
    ":pattern_matching_query_proxy",
  ],
)
