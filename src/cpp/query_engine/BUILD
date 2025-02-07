package(default_visibility = ["//visibility:public"])

cc_library(
    name = "query_engine_lib",
    srcs = glob(["**/*.cc"]),
    hdrs = glob(["**/*.h"]),
    includes = [
        ".",
        "answer_processor",
        "query_element",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    textual_hdrs = [
        "QueryNode.cc",
        "query_element/Iterator.cc",
        "query_element/Sink.cc",
        "query_element/RemoteIterator.cc",
        "query_element/RemoteSink.cc",
    ],
    deps = [
        "//attention_broker:attention_broker_server_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//utils:utils_lib",
        "@mbedtls//:mbedtls",
    ],
)
