load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "inference_agent_lib",
    srcs = ["InferenceProcessor.cc"],
    hdrs = ["InferenceProcessor.h"],
    deps = [
        ":inference_agent",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "inference_agent",
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
        ":inference_proxy",
        ":inference_request",
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//agents/evolution:evolution_lib",
        "//atomdb",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
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
    name = "inference_proxy",
    srcs = ["InferenceProxy.cc"],
    hdrs = ["InferenceProxy.h"],
    deps = [
        "//agents:agents_lib",
    ],
)
