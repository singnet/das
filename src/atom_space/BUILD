load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atom_space_lib",
    srcs = ["AtomSpace.cc"],
    hdrs = ["AtomSpace.h"],
    includes = ["."],
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "//commons:handle_trie",
        "//commons/atoms:atoms_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_singleton",
    ],
)
