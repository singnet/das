load("@pypi//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")
load("@rules_python//python:packaging.bzl", "py_wheel")

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

py_wheel(
    name = "minimal_with_py_library_with_stamp",
    # Package data. We're building "example_minimal_library-0.0.1-py3-none-any.whl"
    distribution = "example_minimal_library{BUILD_USER}",
    python_tag = "py3",
    stamp = 1,
    version = "0.1.{BUILD_TIMESTAMP}",
    deps = [
        ":hyperon_das_atomdb"
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
