load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "redis_mongodb_lib",
    includes = ["."],
    deps = [
        ":redis_context",
        ":redis_context_pool",
        ":redis_mongodb",
        ":redis_mongodb_api_types",
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
        "//commons/atoms:atoms_lib",
        "//atomdb:atomdb_api_types",
    ],
)

cc_library(
    name = "redis_mongodb",
    srcs = ["RedisMongoDB.cc"],
    hdrs = ["RedisMongoDB.h"],
    includes = ["."],
    deps = [
        ":redis_context_pool",
        ":redis_mongodb_api_types",
        "//atomdb",
        "//atomdb:atomdb_cache_singleton",
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
    ],
)
