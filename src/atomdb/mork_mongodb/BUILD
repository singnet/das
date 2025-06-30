load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mork_mongodb_lib",
    includes = ["."],
    deps = [
        ":mork_mongodb",
        ":mork_mongodb_api_types",
    ],
)

cc_library(
    name = "mork_mongodb_api_types",
    srcs = ["MorkMongoDBAPITypes.cc"],
    hdrs = ["MorkMongoDBAPITypes.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_api_types",
    ],
)

cc_library(
    name = "mork_mongodb",
    srcs = ["MorkMongoDB.cc"],
    hdrs = ["MorkMongoDB.h"],
    includes = ["."],
    deps = [
        ":mork_mongodb_api_types",
        "//atomdb",
        "//atomdb:atomdb_cache_singleton",
        "//atomdb/redis_mongodb:redis_mongodb_lib",
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
    ],
)
