load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "db_adapter_lib",
    includes = ["."],
    deps = [
        ":context_loader",
        ":data_mapper",
        ":data_types",
        ":database_connection",
        ":database_wrapper",
        ":pipeline",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//db_adapter/postgres:postgres_lib",
    ],
)

cc_library(
    name = "data_types",
    hdrs = ["DataTypes.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "data_mapper",
    srcs = ["DataMapper.cc"],
    hdrs = ["DataMapper.h"],
    includes = ["."],
    deps = [
        ":data_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "database_wrapper",
    srcs = ["DatabaseWrapper.cc"],
    hdrs = ["DatabaseWrapper.h"],
    includes = ["."],
    deps = [
        ":data_mapper",
        ":data_types",
        ":database_connection",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "context_loader",
    hdrs = ["ContextLoader.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "database_connection",
    srcs = ["DatabaseConnection.cc"],
    hdrs = ["DatabaseConnection.h"],
    includes = ["."],
    deps = [
        ":data_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//commons/processor:processor_lib",
    ],
)

cc_library(
    name = "pipeline",
    srcs = ["Pipeline.cc"],
    hdrs = ["Pipeline.h"],
    includes = ["."],
    deps = [
        ":data_types",
        ":database_wrapper",
        "//atomdb",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//db_adapter/postgres:postgres_lib",
    ],
)
