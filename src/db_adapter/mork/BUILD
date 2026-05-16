load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mork_lib",
    includes = ["."],
    deps = [
        ":mork_connection",
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
