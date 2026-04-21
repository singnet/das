load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

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

cc_library(
    name = "test_atomdb_json_config",
    hdrs = ["TestAtomDBJsonConfig.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)
