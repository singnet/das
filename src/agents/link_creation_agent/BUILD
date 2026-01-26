load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "link_creation_agent_lib",
    srcs = [
        "LinkCreationRequestProcessor.cc",
    ],
    hdrs = [
        "LinkCreationRequestProcessor.h",
    ],
    deps = [
        ":link_creation_agent",
        "//agents:agents_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "link_creation_agent",
    srcs = [
        "LinkCreationAgent.cc",
        "LinkCreationService.cc",
    ],
    hdrs = [
        "LCAQueue.h",
        "LinkCreationAgent.h",
        "LinkCreationService.h",
    ],
    includes = ["."],
    deps = [
        ":link_create_template",
        ":link_creation_agent_request",
        ":processors",
        ":request_proxy",
        ":service",
        "//agents/query_engine:query_engine_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "link_create_template",
    srcs = ["LinkCreateTemplate.cc"],
    hdrs = ["LinkCreateTemplate.h"],
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "service",
    srcs = ["LinkCreationService.cc"],
    hdrs = [
        "LCAQueue.h",
        "LinkCreationService.h",
    ],
    deps = [
        ":link_creation_agent_request",
        ":processors",
        "//agents/query_engine:query_engine_lib",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "processors",
    srcs = [
        "EquivalenceProcessor.cc",
        "ImplicationProcessor.cc",
        "TemplateProcessor.cc",
    ],
    hdrs = [
        "EquivalenceProcessor.h",
        "ImplicationProcessor.h",
        "LinkProcessor.h",
        "TemplateProcessor.h",
    ],
    deps = [
        ":link_create_template",
        ":metta_processor",
        "//agents/query_engine:query_engine_lib",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "metta_processor",
    srcs = [
        "MettaTemplateProcessor.cc",
    ],
    hdrs = [
        "LinkProcessor.h",
        "MettaTemplateProcessor.h",
    ],
    deps = [
        ":link_create_template",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "request_proxy",
    srcs = ["LinkCreationRequestProxy.cc"],
    hdrs = ["LinkCreationRequestProxy.h"],
    deps = [
        "//agents:agents_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "link_creation_agent_request",
    hdrs = ["LinkCreationAgentRequest.h"],
)
