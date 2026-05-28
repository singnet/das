load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test_atomdb_json_config",
    hdrs = ["TestAtomDBJsonConfig.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "mock_animals_data_lib",
    srcs = ["MockAnimalsData.cc"],
    hdrs = ["MockAnimalsData.h"],
    includes = ["."],
    deps = [
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//tests/cpp/test_commons:test_atomdb_json_config",
    ],
)

cc_library(
    name = "test_system_params",
    srcs = ["TestSystemParams.cc"],
    hdrs = ["TestSystemParams.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "@com_github_google_googletest//:gtest",
        "@nlohmann_json//:json",
    ],
)
