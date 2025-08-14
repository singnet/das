load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "benchmark_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":benchmark_runner",
        ":benchmark_utils",
    ],
)

cc_library(
    name = "benchmark_utils",
    srcs = ["benchmark_utils.cc"],
    hdrs = ["benchmark_utils.h"],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "benchmark_runner",
    srcs = ["benchmark_runner.cc"],
    hdrs = ["benchmark_runner.h"],
    deps = [
        ":benchmark_utils",
        "//commons:commons_lib",
    ],
)
