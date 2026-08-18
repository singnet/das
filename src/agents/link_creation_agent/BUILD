load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "link_creation_agent_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":link_creation_processor",
        ":link_creation_proxy",
        "//agents/link_creation_agent/link_creators:link_creators_lib",
    ],
)

cc_library(
    name = "link_creation_proxy",
    srcs = ["LinkCreationProxy.cc"],
    hdrs = ["LinkCreationProxy.h"],
    includes = ["."],
    deps = [
        "//agents:agents_lib",
        "//agents/link_creation_agent/link_creators:link_creators_lib",
        "//agents/query_engine:query_answer",
        "//commons:commons_lib",
        "//attention_broker:attention_broker_client",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "link_creation_processor",
    srcs = ["LinkCreationProcessor.cc"],
    hdrs = ["LinkCreationProcessor.h"],
    includes = ["."],
    deps = [
        ":link_creation_proxy",
        "//agents/query_engine:pattern_matching_query_proxy",
        "//agents/query_engine:query_answer",
        "//attention_broker:attention_broker_client",
        "//commons:commons_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
    ],
)
