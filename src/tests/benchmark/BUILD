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
    name = "atomdb_utils",
    srcs = ["atomdb_utils.cc"],
    hdrs = ["atomdb_utils.h"],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "atomdb_runner",
    hdrs = ["atomdb_runner.h"],
    deps = [
        ":atomdb_utils",
        "//atomdb:atomdb_lib",
    ],
)

cc_library(
    name = "atomdb_operations",
    srcs = ["atomdb_operations.cc"],
    hdrs = ["atomdb_operations.h"],
    deps = [
        ":atomdb_runner",
        ":atomdb_utils",
        "//atomdb:atomdb_lib",
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
        ":atomdb_utils",
        "//atomdb:atomdb_lib",
        "//commons:commons_lib",
        "@mbedtls",
    ],
)
