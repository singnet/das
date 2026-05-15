load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mork_lib",
    includes = ["."],
    deps = [
        ":mork_wrapper",
    ],
)

cc_library(
    name = "mork_wrapper",
    srcs = ["MorkWrapper.cc"],
    hdrs = ["MorkWrapper.h"],
    includes = ["."],
    linkopts = [
        "-L/usr/local/lib",
        "-lpqxx",
        "-lpq",
    ],
    deps = [
        "//atomdb/morkdb:morkdb_lib",
        "//commons:commons_lib",
        "//db_adapter:bounded_shared_queue",
        "//db_adapter:database_connection",
        "//db_adapter:database_mapper",
        "//db_adapter:database_wrapper",
    ],
)
