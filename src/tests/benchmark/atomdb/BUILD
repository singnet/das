load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atomdb_runner",
    srcs = ["atomdb_runner.cc"],
    hdrs = ["atomdb_runner.h"],
    deps = [
        "//atomdb:atomdb_lib",
        "//tests/benchmark:benchmark_lib",
    ],
)

cc_library(
    name = "atomdb_operations",
    srcs = ["atomdb_operations.cc"],
    hdrs = ["atomdb_operations.h"],
    deps = [
        ":atomdb_runner",
        "//atomdb:atomdb_lib",
        "//tests/benchmark:benchmark_lib",
    ],
)

cc_binary(
    name = "atomdb_main",
    srcs = ["atomdb_main.cc"],
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
        ":atomdb_operations",
        ":atomdb_runner",
        "//atomdb:atomdb_lib",
        "//commons:commons_lib",
        "//tests/benchmark:benchmark_lib",
        "@mbedtls",
    ],
)
