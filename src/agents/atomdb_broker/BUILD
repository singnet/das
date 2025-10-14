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
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "atomdb_broker_proxy",
    srcs = ["AtomDBBrokerProxy.cc"],
    hdrs = ["AtomDBBrokerProxy.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_lib",
        "//agents:agents_lib",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
    ],
)
