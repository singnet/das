load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test_commons",
    srcs = ["TestConfig.cc"],
    hdrs = ["TestConfig.h"],
    includes = ["."],
)

cc_library(
    name = "mock_animals_data_lib",
    srcs = ["MockAnimalsData.cc"],
    hdrs = ["MockAnimalsData.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
    ],
)
