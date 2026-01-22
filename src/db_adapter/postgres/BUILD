load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "postgres_lib",
    includes = ["."],
    deps = [
        ":postgres_wrapper",
    ],
)

cc_library(
    name = "postgres_wrapper",
    srcs = ["PostgresWrapper.cc"],
    hdrs = ["PostgresWrapper.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)
