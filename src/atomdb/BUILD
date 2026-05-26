load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atomdb_lib",
    includes = ["."],
    deps = [
        ":atomdb",
        ":atomdb_api_types",
        ":atomdb_singleton",
        ":atomdbutils",
        "//atomdb/adapterdb:adapterdb_lib",
        "//atomdb/inmemorydb:inmemorydb_lib",
        "//atomdb/morkdb:morkdb_lib",
        "//atomdb/redis_mongodb:redis_mongodb_lib",
        "//atomdb/remotedb:remotedb_lib",
    ],
)

cc_library(
    name = "atomdb",
    hdrs = ["AtomDB.h"],
    includes = ["."],
    deps = [
        ":atomdb_api_types",
    ],
)

cc_library(
    name = "atomdbutils",
    srcs = ["AtomDBUtils.cc"],
    hdrs = ["AtomDBUtils.h"],
    includes = ["."],
    deps = [
        ":atomdb",
        ":atomdb_singleton",
    ],
)

cc_library(
    name = "atomdb_api_types",
    hdrs = ["AtomDBAPITypes.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "atomdb_singleton",
    srcs = ["AtomDBSingleton.cc"],
    hdrs = ["AtomDBSingleton.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_api_types",
        "//atomdb/adapterdb",
        "//atomdb/morkdb",
        "//atomdb/redis_mongodb",
        "//atomdb/remotedb:remotedb_lib",
        "//commons:commons_lib",
    ],
)
