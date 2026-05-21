load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mork_lib",
    includes = ["."],
    deps = [
        ":mork_connection",
        ":mork_mapping_strategy",
        ":mork_wrapper",
    ],
)

cc_library(
    name = "mork_wrapper",
    srcs = ["MorkWrapper.cc"],
    hdrs = ["MorkWrapper.h"],
    includes = ["."],
    deps = [
        ":mork_connection",
        "//db_adapter:bounded_shared_queue",
        "//db_adapter:database_types",
        "//db_adapter:database_wrapper",
        "//db_adapter:mapper_factory",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "mork_connection",
    srcs = ["MorkConnection.cc"],
    hdrs = ["MorkConnection.h"],
    includes = ["."],
    deps = [
        "//db_adapter:database_connection",
        "//atomdb/morkdb:morkdb_lib",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "mork_mapping_strategy",
    srcs = ["MorkMappingStrategy.cc"],
    hdrs = ["MorkMappingStrategy.h"],
    includes = ["."],
    deps = [
        ":mork_wrapper",
        "//db_adapter:database_mapping_strategy",
        "//db_adapter:context_loader",
        "//commons:commons_lib",
    ],
)
