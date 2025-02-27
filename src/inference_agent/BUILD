package(default_visibility = ["//visibility:public"])

cc_library(
    name = "inference_agent_lib",
    srcs = glob([ "*.cc"]),
    hdrs = glob(["*.h"]),
    includes = ["."],
    deps = [
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//link_creation_agent:link_creation_agent_lib",
    ],
)
