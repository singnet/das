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
        ":evolution_metta_parser",
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
        "//agents/query_engine:query_answer",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//metta:metta_parser",
    ],
)

cc_library(
    name = "bus_command_router_processor",
    srcs = ["BusCommandRouterProcessor.cc"],
    hdrs = ["BusCommandRouterProcessor.h"],
    includes = ["."],
    deps = [
        ":bus_command_router_http_api_singleton",
        ":bus_command_router_proxy",
        ":evolution_metta_parser",
        "//agents/evolution:query_evolution_proxy",
        "//agents/query_engine:pattern_matching_query_proxy",
        "//agents/query_engine:query_answer",
        "//service_bus:bus_command_processor",
        "//service_bus:service_bus_lib",
        "//service_bus:service_bus_singleton",
    ],
)

cc_library(
    name = "command_router_http_api",
    srcs = ["CommandRouterHttpAPI.cc"],
    hdrs = ["CommandRouterHttpAPI.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons/processor:processor_lib",
    ],
)

cc_library(
    name = "command_router_http_api_singleton",
    srcs = ["CommandRouterHttpAPISingleton.cc"],
    hdrs = ["CommandRouterHttpAPISingleton.h"],
    includes = ["."],
    deps = [
        ":command_router_http_api",
        "//commons:commons_lib",
        "//commons/processor:processor_lib",
    ],
)
