load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "evolution_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":query_evolution_proxy",
        ":query_evolution_processor",
    ],
)

cc_library(
    name = "query_evolution_proxy",
    srcs = ["QueryEvolutionProxy.cc"],
    hdrs = ["QueryEvolutionProxy.h"],
    includes = ["."],
    deps = [
        "//agents/query_engine:query_answer",
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "query_evolution_processor",
    srcs = ["QueryEvolutionProcessor.cc"],
    hdrs = ["QueryEvolutionProcessor.h"],
    includes = ["."],
    deps = [
        ":query_evolution_proxy",
        "//agents/query_engine:query_answer",
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
    ],
)
