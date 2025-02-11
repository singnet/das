load("@rules_python//python:defs.bzl", "py_library")
load("//tools/bazel/pytest:defs.bzl", "py_test")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "helpers",
    srcs = ["helpers.py"],
    data = ["//tests/python/integration/scripts:sh_files"],
)

py_library(
    name = "animals_kb",
    srcs = ["animals_kb.py"],
    deps = ["//hyperon_das_atomdb/utils:expression_hasher"],
)

py_test(
    name = "test_redis_mongo",
    srcs = ["test_redis_mongo.py"],
    deps = [
        ":animals_kb",
        ":helpers",
        "//hyperon_das_atomdb",
        "//hyperon_das_atomdb:database",
        "//hyperon_das_atomdb:exceptions",
        "//hyperon_das_atomdb/utils:expression_hasher",
        "//tests/python:helpers",
    ],
)
