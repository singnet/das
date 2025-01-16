package(default_visibility = ["//visibility:public"])

cc_library(
    name = "query_engine_lib",
    srcs = glob(["*.cc", "query_element/*.cc"]),
    hdrs = glob(["*.h", "query_element/*.h"]),
    includes = [".", "query_element"],
    deps = [
        "//utils:utils_lib",
        "//hasher:hasher_lib",
        "//attention_broker:attention_broker_server_lib",
        "@com_github_singnet_das_node//distributed_algorithm_node:distributed_algorithm_node_lib",
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
