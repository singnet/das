package(default_visibility = ["//visibility:public"])

exports_files(
    [
        ".ruff.toml",
        ".clang-format",
    ],
    visibility = ["//visibility:public"],
)

alias(
    name = "format.check",
    actual = "//tools/format:format.check",
)

alias(
    name = "format",
    actual = "//tools/format",
)

cc_library(
    name = "atomdb_benchmark_utils",
    srcs = ["atomdb_benchmark_utils.cc"],
    hdrs = ["atomdb_benchmark_utils.h"],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_binary(
    name = "atomdb_benchmark",
    srcs = ["atomdb_benchmark.cc"],
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
        ":atomdb_benchmark_utils",
        "//atomdb:atomdb_lib",
        "//commons:commons_lib",
        "@mbedtls",
    ],
)
