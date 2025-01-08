load("@pypi//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",
    srcs = glob(["*.py"]),
)

py_library(
    name = "hyperon_das_atomdb",
    srcs = [
        ":py_files",
        "//hyperon_das_atomdb/adapters:py_files",
        "//hyperon_das_atomdb/utils:py_files",
    ],
    deps = [
        ":database",
        ":exceptions",
        ":index",
        ":logger",
        "//hyperon_das_atomdb/adapters:all_adapters",
        "//hyperon_das_atomdb/utils:expression_hasher",
        "//hyperon_das_atomdb/utils:settings",
        requirement("hyperon_das_atomdb_cpp"),
    ],
)

py_library(
    name = "database",
    srcs = ["database.py"],
    deps = [
        requirement("hyperon_das_atomdb_cpp"),
    ],
)

py_library(
    name = "exceptions",
    srcs = ["exceptions.py"],
    deps = [
        requirement("hyperon_das_atomdb_cpp"),
    ],
)

py_library(
    name = "index",
    srcs = ["index.py"],
    deps = [
        "//hyperon_das_atomdb/utils:expression_hasher",
    ],
)

py_library(
    name = "logger",
    srcs = ["logger.py"],
    deps = [],
)
