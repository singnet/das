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
        "//commons:commons_lib",
        "//db_adapter:bounded_shared_queue",
        "//db_adapter:database_types",
        "//db_adapter:database_wrapper",
        "//db_adapter:mapper_factory",
    ],
)

cc_library(
    name = "mork_connection",
    srcs = ["MorkConnection.cc"],
    hdrs = ["MorkConnection.h"],
    includes = ["."],
    deps = [
        "//atomdb/morkdb:morkdb_lib",
        "//commons:commons_lib",
        "//db_adapter:database_connection",
    ],
)

cc_library(
    name = "mork_mapping_strategy",
    srcs = ["MorkMappingStrategy.cc"],
    hdrs = ["MorkMappingStrategy.h"],
    includes = ["."],
    deps = [
        ":mork_wrapper",
        "//commons:commons_lib",
        "//db_adapter:context_loader",
        "//db_adapter:database_mapping_strategy",
    ],
)
