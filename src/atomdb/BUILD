load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atomdb_lib",
    includes = ["."],
    deps = [
        ":atomdb",
        ":atomdb_api_types",
        ":atomdb_cache",
        ":atomdb_cache_singleton",
        ":atomdb_singleton",
        "//atomdb/morkdb:morkdb_lib",
        "//atomdb/redis_mongodb:redis_mongodb_lib",
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
        ":atomdb_cache_singleton",
        "//atomdb:atomdb_api_types",
        "//atomdb/morkdb",
        "//atomdb/redis_mongodb",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "atomdb_cache",
    srcs = ["AtomDBCache.cc"],
    hdrs = ["AtomDBCache.h"],
    includes = ["."],
    deps = [
        ":atomdb_api_types",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "atomdb_cache_singleton",
    srcs = ["AtomDBCacheSingleton.cc"],
    hdrs = ["AtomDBCacheSingleton.h"],
    includes = ["."],
    deps = [
        ":atomdb_cache",
        "//commons:commons_lib",
    ],
)
