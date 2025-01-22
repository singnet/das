package(default_visibility = ["//visibility:public"])

cc_library(
    name = "link_creation_agent_lib",
    srcs = glob([ "src/das_node/*.cc"]),
    hdrs = glob(["include/*.h", "include/das_node/*.h", "include/das_node/das_server/*.h", "include/das_node/link_creation/*.h", "include/das_node/query/*.h", "include/utils/*.h"]),
    includes = [".", "include"],
    deps = [
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
    ],
)
