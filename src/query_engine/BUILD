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
    "//commons:commons_lib",
    "//distributed_algorithm_node:distributed_algorithm_node_lib",
  ],
)
