load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hasher_lib",
    srcs = ["expression_hasher.cc"],
    hdrs = [
        "Hasher.h",
        "expression_hasher.h",
    ],
    includes = ["."],
    deps = [
        "@mbedtls",
    ],
)
