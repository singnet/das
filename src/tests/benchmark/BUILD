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

cc_binary(
    name = "atomdb_benchmark",
    srcs = ["atomdb_benchmark.cc"],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    deps = [
        "//commons:commons_lib",
        "//atomdb:atomdb_singleton",
        "@mbedtls",
    ]
)
