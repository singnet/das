load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "inference_agent_lib",
    srcs = [
        "InferenceAgent.cc",
    ],
    hdrs = [
        "InferenceAgent.h",
        "InferenceIterator.h",
        "InferenceRequestValidator.h",
    ],
    includes = ["."],
    deps = [
        ":distributed_inference_control_agent_node",
        ":inference_agent_node",
        ":inference_request",
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//atomdb",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        ":inference_bus_processor",

    ],
)

cc_library(
    name = "distributed_inference_control_agent_node",
    srcs = ["EvolutionAgentNode.cc"],
    hdrs = ["EvolutionAgentNode.h"],
    deps = [
        "//distributed_algorithm_node:star_node",
    ],
)

cc_library(
    name = "inference_request",
    srcs = ["InferenceRequest.cc"],
    hdrs = ["InferenceRequest.h"],
    deps = [
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "inference_agent_node",
    srcs = ["InferenceAgentNode.cc"],
    hdrs = ["InferenceAgentNode.h"],
    deps = [
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//commons:commons_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
    ],
)

cc_library(
    name = "inference_proxy",
    srcs = ["InferenceProxy.cc"],
    hdrs = ["InferenceProxy.h"],
    deps = [
        "//agents:agents_lib",
    ],
)

cc_library(
    name = "inference_bus_processor",
    srcs = ["InferenceProcessor.cc", "InferenceAgent.cc"],
    hdrs = ["InferenceProcessor.h", "InferenceAgent.h"],
    deps = [
        "//service_bus:service_bus_lib",
        ":inference_proxy",
        ":distributed_inference_control_agent_node",
        ":inference_agent_node",
        ":inference_request",
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//atomdb",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
    ],
)