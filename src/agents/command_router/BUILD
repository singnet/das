load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "command_router_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":bus_command_router_processor",
        ":bus_command_router_proxy",
        ":command_line_parser",
        ":evolution_metta_parser",
    ],
)

cc_library(
    name = "command_line_parser",
    srcs = ["CommandLineParser.cc"],
    hdrs = ["CommandLineParser.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "bus_command_router_proxy",
    srcs = ["BusCommandRouterProxy.cc"],
    hdrs = ["BusCommandRouterProxy.h"],
    includes = ["."],
    deps = [
        "//agents:base_query_proxy",
        "//agents/evolution:query_evolution_proxy",
        "//agents/query_engine:pattern_matching_query_proxy",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "evolution_metta_parser",
    srcs = ["EvolutionMettaParser.cc"],
    hdrs = ["EvolutionMettaParser.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "bus_command_router_processor",
    srcs = ["BusCommandRouterProcessor.cc"],
    hdrs = ["BusCommandRouterProcessor.h"],
    includes = ["."],
    deps = [
        ":bus_command_router_proxy",
        ":command_line_parser",
        ":evolution_metta_parser",
        "//agents/evolution:query_evolution_proxy",
        "//agents/query_engine:pattern_matching_query_proxy",
        "//agents/query_engine:query_answer",
        "//service_bus:bus_command_processor",
        "//service_bus:service_bus_lib",
        "//service_bus:service_bus_singleton",
    ],
)
