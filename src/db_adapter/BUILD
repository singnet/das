load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "db_adapter_lib",
    includes = ["."],
    deps = [
        ":postgres",
    ],
)

cc_library(
    name = "postgres",
    hdrs = ["Postgres.h"],
    includes = ["."],
    deps = [
        ":atomdb_api_types",
    ],
)