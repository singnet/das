load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "postgres_lib",
    includes = ["."],
    deps = [
        ":postgres_connection",
        ":postgres_mapping_strategy",
        ":postgres_wrapper",
    ],
)

cc_library(
    name = "postgres_connection",
    srcs = ["PostgresConnection.cc"],
    hdrs = ["PostgresConnection.h"],
    includes = ["."],
    linkopts = [
        "-L/usr/local/lib",
        "-lpqxx",
        "-lpq",
    ],
    deps = [
        "//commons:commons_lib",
        "//db_adapter:database_connection",
    ],
)

cc_library(
    name = "postgres_wrapper",
    srcs = ["PostgresWrapper.cc"],
    hdrs = ["PostgresWrapper.h"],
    includes = ["."],
    deps = [
        ":postgres_connection",
        "//commons:commons_lib",
        "//db_adapter:bounded_shared_queue",
        "//db_adapter:database_mapper",
        "//db_adapter:database_wrapper",
        "//db_adapter:mapper_factory",
        "//db_adapter/sql:sql_wrapper",
    ],
)

cc_library(
    name = "postgres_mapping_strategy",
    srcs = ["PostgresMappingStrategy.cc"],
    hdrs = ["PostgresMappingStrategy.h"],
    includes = ["."],
    deps = [
        ":postgres_wrapper",
        "//commons:commons_lib",
        "//db_adapter:bounded_shared_queue",
        "//db_adapter:context_loader",
        "//db_adapter:database_mapping_strategy",
        "//db_adapter:database_types",
    ],
)
