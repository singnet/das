load("@pypi_dev//:requirements.bzl", dev_requirement = "requirement")
load("@rules_python//python:defs.bzl", "py_library")
load("//tools/bazel/pytest:defs.bzl", "py_test")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "fixtures",
    srcs = ["fixtures.py"],
    deps = [
        "//hyperon_das_atomdb/adapters:all_adapters",
        dev_requirement("mongomock"),
    ],
)

py_test(
    name = "test_database_public_methods",
    srcs = ["test_database_public_methods.py"],
    data = ["//tests/unit/adapters/data:json_data"],
    deps = [
        ":fixtures",
        "//hyperon_das_atomdb:database",
        "//tests:helpers",
    ],
)

py_test(
    name = "test_database_private_methods",
    srcs = ["test_database_private_methods.py"],
    deps = [
        ":fixtures",
        "//hyperon_das_atomdb:database",
        "//hyperon_das_atomdb:exceptions",
        "//tests:helpers",
    ],
)
