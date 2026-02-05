load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "inmemorydb_lib",
    includes = ["."],
    deps = [
        ":inmemorydb",
        ":inmemorydb_api_types",
    ],
)

cc_library(
    name = "inmemorydb_api_types",
    srcs = ["InMemoryDBAPITypes.cc"],
    hdrs = ["InMemoryDBAPITypes.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_api_types",
    ],
)

cc_library(
    name = "inmemorydb",
    srcs = ["InMemoryDB.cc"],
    hdrs = ["InMemoryDB.h"],
    includes = ["."],
    deps = [
        ":inmemorydb_api_types",
        "//atomdb",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)
