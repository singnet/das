load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atom_space_lib",
    srcs = [
        "AtomSpace.cc",
    ],
    hdrs = [
        "AtomSpace.h",
    ],
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//atomdb",
        "//commons:handle_trie",
        "//service_bus:service_bus_lib",
    ],
)
