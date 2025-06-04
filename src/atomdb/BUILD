package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atomdb",
    hdrs = ["AtomDB.h"],
    deps = [
        ":atomdb_api_types",
    ],
)

cc_library(
    name = "atomdb_api_types",
    hdrs = ["AtomDBAPITypes.h"],
    deps = [
        "//commons:commons_lib",
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "redis_mongodb_api_types",
    srcs = ["RedisMongoDBAPITypes.cc"],
    hdrs = ["RedisMongoDBAPITypes.h"],
    deps = [
        ":atomdb_api_types",
    ],
)

cc_library(
    name = "atomdb_singleton",
    srcs = ["AtomDBSingleton.cc"],
    hdrs = ["AtomDBSingleton.h"],
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
        ":redis_mongodb_api_types",
        "//attention_broker:attention_broker_server_lib",
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
