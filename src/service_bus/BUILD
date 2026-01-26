load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "service_bus_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":bus_command_processor",
        ":bus_command_proxy",
        ":service_bus",
        ":service_bus_singleton",
        "//commons:commons_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
    ],
)

cc_library(
    name = "service_bus_singleton",
    srcs = ["ServiceBusSingleton.cc"],
    hdrs = ["ServiceBusSingleton.h"],
    includes = ["."],
    deps = [
        ":service_bus",
        "//commons:commons_lib",
        "//distributed_algorithm_node:star_node",
    ],
)

cc_library(
    name = "port_pool",
    srcs = ["PortPool.cc"],
    hdrs = ["PortPool.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "service_bus",
    srcs = ["ServiceBus.cc"],
    hdrs = ["ServiceBus.h"],
    includes = ["."],
    deps = [
        ":bus_command_processor",
        ":port_pool",
        "//commons:commons_lib",
        "//distributed_algorithm_node:bus_node",
    ],
)

cc_library(
    name = "bus_command_proxy",
    srcs = ["BusCommandProxy.cc"],
    hdrs = ["BusCommandProxy.h"],
    includes = ["."],
    deps = [
        ":port_pool",
        "//commons:commons_lib",
        "//distributed_algorithm_node:bus_node",
        "//distributed_algorithm_node:star_node",
    ],
)

cc_library(
    name = "bus_command_processor",
    srcs = ["BusCommandProcessor.cc"],
    hdrs = ["BusCommandProcessor.h"],
    includes = ["."],
    deps = [
        ":bus_command_proxy",
        "//commons:commons_lib",
        "//distributed_algorithm_node:bus_node",
    ],
)
