load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atom_space_lib",
    srcs = ["AtomSpace.cc"],
    hdrs = ["AtomSpace.h"],
    includes = ["."],
    deps = [
        ":link_template_interface",
        "//agents/query_engine:query_engine_lib",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
        "//commons/atoms:atoms_lib",
        "//commons:commons_lib",
        "//commons:handle_trie",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_singleton",
    ],
)

cc_library(
    name = "link_template_interface",
    hdrs = ["LinkTemplateInterface.h"],
    includes = ["."],
    deps = [],
)
