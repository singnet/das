load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hasher_lib",
    srcs = ["expression_hasher.cc"],
    hdrs = ["expression_hasher.h", "Hasher.h"],
    includes = ["."],
    deps = [
        "@mbedtls",
    ],
)
