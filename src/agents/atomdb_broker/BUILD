load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atomdb_broker_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":atomdb_broker_processor",
        ":atomdb_broker_proxy",
    ],
)

cc_library(
    name = "atomdb_broker_processor",
    srcs = ["AtomDBBrokerProcessor.cc"],
    hdrs = ["AtomDBBrokerProcessor.h"],
    includes = ["."],
    deps = [
        ":atomdb_broker_proxy",
    ],
)

cc_library(
    name = "atomdb_broker_proxy",
    srcs = ["AtomDBBrokerProxy.cc"],
    hdrs = ["AtomDBBrokerProxy.h"],
    includes = ["."],
    deps = [
        "//agents:base_proxy",
        "//atomdb:atomdb_singleton",
        "//atomdb:atomdb",
        "//agents/query_engine:query_engine_lib",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//service_bus:service_bus_lib",
    ],
)
