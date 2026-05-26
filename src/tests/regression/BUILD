load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "regression_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":adapterdb_main",
    ],
)

cc_binary(
    name = "adapterdb_main",
    srcs = ["adapterdb_main.cc"],
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
        "//atomdb",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "@mbedtls",
    ],
)
