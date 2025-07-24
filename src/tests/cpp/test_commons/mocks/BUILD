load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mock_service_bus_lib",
    srcs = [],
    hdrs = ["MockServiceBus.h"],
    includes = ["."],
    deps = [
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "mock_atom_db_lib",
    srcs = [],
    hdrs = ["MockAtomDB.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_lib",
    ],
)
