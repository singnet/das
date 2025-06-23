load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "link_creation_agent_lib",
    srcs = [
        "LCALink.cc",
        "LinkCreationAgent.cc",
        "LinkCreationService.cc",
    ],
    hdrs = [
        "LCALink.h",
        "LinkCreationAgent.h",
        "LinkCreationService.h",
    ],
    includes = ["."],
    deps = [
        ":das_agent_node",
        ":link",
        ":link_create_template",
        ":link_creation_agent_node",
        ":link_creation_db_helper",
        ":processors",
        ":request_processor",
        ":service",
        "//agents/query_engine:query_engine_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "das_agent_node",
    srcs = ["DASAgentNode.cc"],
    hdrs = [
        "DASAgentNode.h",
        "LCAQueue.h",
    ],
    deps = [
        ":link_creation_db_helper",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
    ],
)

cc_library(
    name = "link",
    srcs = ["LCALink.cc"],
    hdrs = [
        "LCALink.h",
        "LinkCreateTemplate.h",
    ],
    deps = [
        "//agents/query_engine:query_engine_lib",
    ],
)

cc_library(
    name = "link_create_template",
    srcs = ["LinkCreateTemplate.cc"],
    hdrs = ["LinkCreateTemplate.h"],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "link_creation_agent_node",
    srcs = ["LinkCreationAgentNode.cc"],
    hdrs = [
        "DummyMessage.h",
        "LCAQueue.h",
        "LinkCreationAgentNode.h",
    ],
    deps = [
        "//distributed_algorithm_node:star_node",
    ],
)

cc_library(
    name = "link_creation_db_helper",
    srcs = ["LinkCreationDBHelper.cc"],
    hdrs = ["LinkCreationDBHelper.h"],
    deps = [
        ":link",
        "//atomdb",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
    ],
)

cc_library(
    name = "service",
    srcs = ["LinkCreationService.cc"],
    hdrs = [
        "LinkCreationService.h",
        "ThreadPool.h",
    ],
    deps = [
        ":das_agent_node",
        ":link",
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
        ":link",
        ":link_create_template",
        ":link_creation_db_helper",
        "//agents/query_engine:query_engine_lib",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "request_processor",
    srcs = [
        "LinkCreationAgent.cc",
        "LinkCreationRequestProcessor.cc",
    ],
    hdrs = [
        "LinkCreationAgent.h",
        "LinkCreationRequestProcessor.h",
    ],
    deps = [
        ":das_agent_node",
        ":link",
        ":link_create_template",
        ":link_creation_agent_node",
        ":link_creation_db_helper",
        ":processors",
        ":request_proxy",
        ":service",
        "//agents/query_engine:query_engine_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "request_proxy",
    srcs = ["LinkCreationRequestProxy.cc"],
    hdrs = ["LinkCreationRequestProxy.h"],
    deps = [
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
    ],
)
