load("@pypi//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",
    srcs = glob(["*.py"]),
)

py_library(
    name = "utils",
    srcs = [":py_files"],
    deps = [
        ":expression_hasher",
        ":settings",
    ],
)

py_library(
    name = "expression_hasher",
    srcs = ["expression_hasher.py"],
    deps = [],
)

py_library(
    name = "settings",
    srcs = ["settings.py"],
    deps = [
        requirement("python-dotenv"),
    ],
)
