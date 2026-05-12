load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "adapterdb_lib",
    includes = ["."],
    deps = [
        ":adapterdb",
    ],
)

cc_library(
    name = "adapterdb",
    srcs = ["AdapterDB.cc"],
    hdrs = ["AdapterDB.h"],
    includes = ["."],
    deps = [
        "//atomdb",
        "//atomdb/morkdb",
        "//atomdb/redis_mongodb",
        "//atomdb/remotedb:remotedb_lib",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//db_adapter:db_adapter_lib",
    ],
)
