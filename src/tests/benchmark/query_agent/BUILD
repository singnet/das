load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "query_agent_runner",
    srcs = ["query_agent_runner.cc"],
    hdrs = ["query_agent_runner.h"],
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//atom_space:atom_space_lib",
        "//atomdb:atomdb_lib",
        "//atomdb:atomdb_singleton",
        "//tests/benchmark:benchmark_lib",
    ],
)

cc_library(
    name = "query_agent_operations",
    srcs = ["query_agent_operations.cc"],
    hdrs = ["query_agent_operations.h"],
    deps = [
        ":query_agent_runner",
        "//atomdb:atomdb_lib",
        "//tests/benchmark:benchmark_lib",
    ],
)

cc_binary(
    name = "query_agent_main",
    srcs = ["query_agent_main.cc"],
    defines = ["BAZEL_BUILD"],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        ":query_agent_operations",
        ":query_agent_runner",
        "//agents/query_engine:query_engine_lib",
        "//atom_space:atom_space_lib",
        "//atomdb:atomdb_lib",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
        "//tests/benchmark:benchmark_lib",
        "@mbedtls",
    ],
)
