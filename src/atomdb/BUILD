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
        ":redis_context",
        ":redis_context_pool",
        ":redis_mongodb",
        ":redis_mongodb_api_types",
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
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "redis_context",
    srcs = ["RedisContext.cc"],
    hdrs = ["RedisContext.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "redis_context_pool",
    srcs = ["RedisContextPool.cc"],
    hdrs = ["RedisContextPool.h"],
    includes = ["."],
    deps = [
        ":redis_context",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "redis_mongodb_api_types",
    srcs = ["RedisMongoDBAPITypes.cc"],
    hdrs = ["RedisMongoDBAPITypes.h"],
    includes = ["."],
    deps = [
        ":atomdb_api_types",
    ],
)

cc_library(
    name = "atomdb_singleton",
    srcs = ["AtomDBSingleton.cc"],
    hdrs = ["AtomDBSingleton.h"],
    includes = ["."],
    deps = [
        ":atomdb_cache_singleton",
        ":redis_mongodb",
        "//atomdb:atomdb_api_types",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "redis_mongodb",
    srcs = ["RedisMongoDB.cc"],
    hdrs = ["RedisMongoDB.h"],
    includes = ["."],
    deps = [
        ":atomdb",
        ":atomdb_cache_singleton",
        ":redis_context_pool",
        ":redis_mongodb_api_types",
        "//attention_broker:attention_broker_lib",
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
