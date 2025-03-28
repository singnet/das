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
  srcs = ["AtomDBAPITypes.cc"],
  hdrs = ["AtomDBAPITypes.h"],
  deps = [
    "//hasher:hasher_lib",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "atomdb_singleton",
  srcs = ["AtomDBSingleton.cc"],
  hdrs = ["AtomDBSingleton.h"],
  deps = [
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
    ":atomdb_api_types",
    "//attention_broker:attention_broker_server_lib",
    "//commons:commons_lib",
  ],
)
