package(default_visibility = ["//visibility:public"])

cc_library(
    name = "query_engine_lib",
    srcs = ["DASNode.cc"],
    hdrs = ["DASNode.h"],
    includes = ["."],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    textual_hdrs = ["QueryNode.cc"],
    deps = [
        ":count_answer",
        ":handles_answer",
        ":query_answer",
        ":query_node",
        ":pattern_matching_query_processor",
        ":pattern_matching_query_proxy",
        "//distributed_algorithm_node:star_node",
        "//query_engine/query_element:query_element_lib",
        "//query_engine/answer_processor:answer_processor_lib",
        "//attention_broker:attention_broker_server_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//commons:commons_lib",
        "@mbedtls//:mbedtls",
    ],
)

cc_library(
  name = "count_answer",
  hdrs = ["CountAnswer.h"],
)

cc_library(
  name = "query_answer",
  hdrs = ["QueryAnswer.h"],
)

cc_library(
  name = "handles_answer",
  srcs = ["HandlesAnswer.cc"],
  hdrs = ["HandlesAnswer.h"],
  deps = [
    ":query_answer",
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
    ":handles_answer",
    ":count_answer",
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
    ":query_node",
    ":handles_answer",
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
