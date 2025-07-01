load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atoms_lib",
    srcs = [
        "Atom.cc",
        "Node.cc",
    ],
    hdrs = [
        "Atom.h",
        "Node.h",
        "HandleDecoder.h",
    ],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//hasher:hasher_lib",
        "//commons:handle_trie",
    ],
)
