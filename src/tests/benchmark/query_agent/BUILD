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
    name = "query_agent_utils",
    srcs = ["query_agent_utils.cc"],
    hdrs = ["query_agent_utils.h"],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "query_agent_runner",
    hdrs = ["query_agent_runner.h"],
    deps = [
        ":query_agent_utils",
        "//atomdb:query_agent_lib",
    ],
)

cc_library(
    name = "query_agent_operations",
    srcs = ["query_agent_operations.cc"],
    hdrs = ["query_agent_operations.h"],
    deps = [
        ":query_agent_runner",
        ":query_agent_utils",
        "//atomdb:query_agent_lib",
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
        ":query_agent_utils",
        "//atomdb:query_agent_lib",
        "//commons:commons_lib",
        "@mbedtls",
    ],
)
