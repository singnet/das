load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "atoms_lib",
    srcs = [
        "Atom.cc",
        "Link.cc",
        "LinkSchema.cc",
        "Node.cc",
        "UntypedVariable.cc",
        "Wildcard.cc",
    ],
    hdrs = [
        "Atom.h",
        "HandleDecoder.h",
        "Link.h",
        "LinkSchema.h",
        "Node.h",
        "UntypedVariable.h",
        "Wildcard.h",
    ],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons:handle_trie",
        "//metta:metta_mapping",
        "//hasher:hasher_lib",
    ],
)
