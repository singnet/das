load("@rules_python//python:defs.bzl", "py_library")
load("@pypi//:requirements.bzl", "requirement")


package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",
    srcs = glob(["*.py"]),
)

filegroup(
    name = "py_all_files",
    srcs = glob(["**/*.py"]),
)

py_library(
    name = "evolution",
    srcs = [":py_all_files"],
    deps = [
        ":fitness_functions",
        ":selection_methods",
        ":utils",
        ":optimizer",
        "//evolution/das_node",
    ],
)

py_library(
    name = "fitness_functions",
    srcs = ["fitness_functions.py"],
    deps = [],
)

py_library(
    name = "selection_methods",
    srcs = ["selection_methods.py"],
    deps = [],
)

py_library(
    name = "utils",
    srcs = ["utils.py"],
    deps = [],
)

py_library(
    name = "optimizer",
    srcs = ["optimizer.py"],
    deps = [
        ":fitness_functions",
        ":selection_methods",
        ":utils",
        "//evolution/das_node:das_node",
        "//evolution/das_node:query_answer",
    ],
)