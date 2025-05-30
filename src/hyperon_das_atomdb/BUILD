load("@pypi_atomdb//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")
load("@rules_python//python:packaging.bzl", "py_package", "py_wheel")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_all_files",  # all python files in the project, including subdirectories
    srcs = glob(["**/*.py"]),
)

py_library(
    name = "hyperon_das_atomdb",
    srcs = [":py_all_files"],
    deps = [
        ":database",
        ":exceptions",
        ":index",
        ":logger",
        "//hyperon_das_atomdb/adapters",
        "//hyperon_das_atomdb/utils",
        requirement("hyperon_das_atomdb_cpp"),
    ],
)

py_package(
    name = "hyperon_das_atomdb_package",
    deps = [":hyperon_das_atomdb"],
)

py_wheel(
    name = "hyperon_das_atomdb_wheel",
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
    distribution = "hyperon_das_atomdb",
    platform = "any",
    python_requires = ">=3.10",
    python_tag = "py3",
    requires_file = "//deps:requirements_atomdb_lock.txt",
    stamp = 1,
    summary = "Persistence layer for Distributed AtomSpace",
    version = "$(DAS_VERSION)",  # must be defined when calling `bazel build` with `--define=DAS_VERSION=<version>`
    deps = [":hyperon_das_atomdb_package"],
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
