load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "sql_lib",
    includes = ["."],
    deps = [
        ":sql2atoms_mapper",
        ":sql_wrapper",
        "//db_adapter/sql/postgres:postgres_lib",
    ],
)

cc_library(
    name = "sql2atoms_mapper",
    srcs = ["Sql2AtomsMapper.cc"],
    hdrs = ["Sql2AtomsMapper.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//db_adapter:database_mapper",
        "//db_adapter:database_types",
        "//metta:metta_lib",
    ],
)

cc_library(
    name = "sql_wrapper",
    srcs = ["SqlWrapper.cc"],
    hdrs = ["SqlWrapper.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//db_adapter:database_connection",
        "//db_adapter:database_types",
        "//db_adapter:database_wrapper",
    ],
)
