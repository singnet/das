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
        "//service_bus:service_bus_lib",
        "//agents:agents_lib",
        ":link_creation_agent",
    ],
)

cc_library(
    name = "link_creation_agent",
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
        ":link",
        ":link_create_template",
        ":link_creation_db_helper",
        ":processors",
        ":service",
        ":request_proxy",
        "//agents/query_engine:query_engine_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
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
