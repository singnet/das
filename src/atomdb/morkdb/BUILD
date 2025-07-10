load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "morkdb_lib",
    includes = ["."],
    deps = [
        ":morkdb",
        ":morkdb_api_types",
    ],
)

cc_library(
    name = "morkdb_api_types",
    srcs = ["MorkDBAPITypes.cc"],
    hdrs = ["MorkDBAPITypes.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_api_types",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "morkdb",
    srcs = ["MorkDB.cc"],
    hdrs = ["MorkDB.h"],
    includes = ["."],
    deps = [
        ":morkdb_api_types",
        "//atomdb",
        "//atomdb:atomdb_cache_singleton",
        "//atomdb/redis_mongodb:redis_mongodb_lib",
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)
