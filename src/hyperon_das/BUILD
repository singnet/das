load("@pypi//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")
load("@rules_python//python:packaging.bzl", "py_wheel")
load("@rules_python//python:packaging.bzl", "py_package")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",  # python files in this directory
    srcs = glob(["*.py"]),
)

filegroup(
    name = "py_all_files",  # all python files in the project, including subdirectories
    srcs = glob(["**/*.py"]),
)

py_library(
    name = "hyperon_das",
    srcs = [":py_all_files"],
    deps = [
        ":client",
        ":constants",
        ":context",
        ":das",
        ":decorators",
        ":exceptions",
        ":link_filters",
        ":logger",
        ":traverse_engines",
        ":type_alias",
        ":utils",
    ],
)

py_package(
  name = "hyperon_das_package",
  deps = [":hyperon_das"],
)

py_wheel(
    name = "hyperon_das_wheel",
    abi = "none",
    author = "Andre Senna",
    author_email = "andre@singularitynet.io",
    classifiers = [
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
    ],
    description_content_type = "text/markdown",
    description_file = "README.md",
    distribution = "hyperon_das",
    platform = "any",
    python_requires = ">=3.10",
    python_tag = "py3",
    requires_file = "//deps:requirements_lock.txt",
    stamp = 1,
    summary = "Query Engine API for Distributed AtomSpace",
    version = "$(DAS_VERSION)",  # must be defined when calling `bazel build` with `--define=DAS_VERSION=<version>`
    deps = [
        ":hyperon_das_package",
    ],
)

py_library(
    name = "das",
    srcs = ["das.py"],
    deps = [
        ":constants",
        ":context",
        ":exceptions",
        ":link_filters",
        ":logger",
        ":traverse_engines",
        ":type_alias",
        ":utils",
        "//hyperon_das/cache:cache_controller",
        "//hyperon_das/query_engines:local_query_engine",
        "//hyperon_das/query_engines:remote_query_engine",
        "//hyperon_das_atomdb",
    ],
)

py_library(
    name = "client",
    srcs = ["client.py"],
    deps = [
        ":exceptions",
        ":link_filters",
        ":logger",
        ":type_alias",
        ":utils",
        "//hyperon_das_atomdb",
        requirement("requests"),
    ],
)

py_library(
    name = "traverse_engines",
    srcs = ["traverse_engines.py"],
    deps = [
        "//hyperon_das/cache",
        "//hyperon_das/cache:iterators",
        "//hyperon_das_atomdb",
    ],
)

py_library(
    name = "context",
    srcs = ["context.py"],
    deps = [
        ":utils",
        "//hyperon_das_atomdb",
    ],
)

py_library(
    name = "utils",
    srcs = ["utils.py"],
    deps = [
        ":decorators",
        ":exceptions",
        ":logger",
        requirement("requests"),
    ],
)

py_library(
    name = "decorators",
    srcs = ["decorators.py"],
    deps = [
        ":exceptions",
        ":logger",
    ],
)

py_library(
    name = "constants",
    srcs = ["constants.py"],
    deps = [],
)

py_library(
    name = "exceptions",
    srcs = ["exceptions.py"],
    deps = [],
)

py_library(
    name = "logger",
    srcs = ["logger.py"],
    deps = [],
)

py_library(
    name = "type_alias",
    srcs = ["type_alias.py"],
    deps = [],
)

py_library(
    name = "link_filters",
    srcs = ["link_filters.py"],
    deps = [
        "//hyperon_das_atomdb",
    ],
)
