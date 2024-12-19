load("@python_deps//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "hyperon_das_atomdb",
    srcs = glob(["*.py"]),
    deps = [
        ":database",
        ":exceptions",
        ":index",
        ":logger",
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
        "//python/hyperon_das_atomdb/utils:expression_hasher",
    ],
)

py_library(
    name = "logger",
    srcs = ["logger.py"],
    deps = [],
)
