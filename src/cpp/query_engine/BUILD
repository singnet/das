package(default_visibility = ["//visibility:public"])

cc_library(
    name = "query_engine_lib",
    srcs = glob(["**/*.cc"]),
    hdrs = glob(["**/*.h"]),
    includes = [".", "query_element", "answer_processor"],
    deps = [
        "//utils:utils_lib",
        "//hasher:hasher_lib",
        "//attention_broker:attention_broker_server_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
    ],
    linkopts = [
        "-lmbedcrypto",
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],  

)
