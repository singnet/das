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

py_binary(
    name = "main",
    srcs = ["main.py"],
    deps = [
      ":optimizer"
    ],
)

py_library(
    name = "fitness_functions",
    srcs = ["fitness_functions.py"],
    deps = [
      "//hyperon_das_atomdb:exceptions",
    ],
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
        "//hyperon_das:hyperon_das",
        "//hyperon_das_atomdb:hyperon_das_atomdb",
        "//hyperon_das_query_engine:hyperon_das_query_engine",
        # "//evolution/das_node:das_node",
        # "//evolution/das_node:query_answer",
    ],
)
