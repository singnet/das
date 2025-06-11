load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "inference_agent_lib",
    srcs = [
        "inference_agent.cc",
    ],
    hdrs = [
        "inference_agent.h",
        "inference_iterator.h",
        "inference_request_validator.h",
    ],
    includes = ["."],
    deps = [
        ":distributed_inference_control_agent_node",
        ":inference_agent_node",
        ":inference_request",
        "//atomdb",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "//link_creation_agent:link_creation_agent_lib",
    ],
)

cc_library(
    name = "distributed_inference_control_agent_node",
    srcs = ["distributed_inference_control_agent_node.cc"],
    hdrs = ["distributed_inference_control_agent_node.h"],
    deps = [
        "//distributed_algorithm_node:star_node",
    ],
)

cc_library(
    name = "inference_request",
    srcs = ["inference_request.cc"],
    hdrs = ["inference_request.h"],
    deps = [
        "//commons:commons_lib",
        "//link_creation_agent:link_creation_agent_lib",
    ],
)

cc_library(
    name = "inference_agent_node",
    srcs = ["inference_agent_node.cc"],
    hdrs = ["inference_agent_node.h"],
    deps = [
        "//commons:commons_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//link_creation_agent:link_creation_agent_lib",
    ],
)
