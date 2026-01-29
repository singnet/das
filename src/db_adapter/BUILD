load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "db_adapter_lib",
    includes = ["."],
    deps = [
        ":data_mapper",
        ":data_types",
        ":db_wrapper",
        "//db_adapter/postgres:postgres_lib",
    ],
)

cc_library(
    name = "data_mapper",
    srcs = ["DataMapper.cc"],
    hdrs = ["DataMapper.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
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
    name = "db_wrapper",
    hdrs = ["DBWrapper.h"],
    includes = ["."],
    deps = [
        ":data_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)
