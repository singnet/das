load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "remotedb_lib",
    includes = ["."],
    deps = [
        ":remotedb",
    ],
)

cc_library(
    name = "remotedb",
    srcs = [
        "RemoteAtomDB.cc",
        "RemoteAtomDBPeer.cc",
    ],
    hdrs = [
        "RemoteAtomDB.h",
        "RemoteAtomDBPeer.h",
    ],
    includes = ["."],
    deps = [
        "//atomdb",
        "//atomdb:atomdb_api_types",
        "//atomdb/inmemorydb",
        "//atomdb/inmemorydb:inmemorydb_api_types",
        "//atomdb/morkdb:morkdb_lib",
        "//atomdb/redis_mongodb:redis_mongodb_lib",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//hasher:hasher_lib",
        "@nlohmann_json//:json",
    ],
)
