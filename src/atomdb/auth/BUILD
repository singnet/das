load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "protected_atomdb_lib",
    includes = ["."],
    deps = [
        ":protected_atomdb",
    ],
)

cc_library(
    name = "protected_atomdb",
    srcs = ["ProtectedAtomDB.cc"],
    hdrs = ["ProtectedAtomDB.h"],
    includes = ["."],
    deps = [
        "//atomdb",
        "//atomdb:atomdb_api_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)
