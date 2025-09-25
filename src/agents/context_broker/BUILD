load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "context_broker_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":context_broker_processor",
        ":context_broker_proxy",
    ],
)

cc_library(
    name = "context_broker_processor",
    srcs = ["ContextBrokerProcessor.cc"],
    hdrs = ["ContextBrokerProcessor.h"],
    includes = ["."],
    deps = [
        ":context_broker_proxy",
        "//agents/query_engine:pattern_matching_query_proxy",
        "//agents/query_engine:query_answer",
        "//atom_space:atom_space_lib",
        "//attention_broker:attention_broker_client",
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "context_broker_proxy",
    srcs = ["ContextBrokerProxy.cc"],
    hdrs = ["ContextBrokerProxy.h"],
    includes = ["."],
    deps = [
        "//agents:agents_lib",
        "//agents/query_engine:query_answer",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
    ],
)
