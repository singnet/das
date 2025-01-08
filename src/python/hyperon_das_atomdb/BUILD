load("@pypi//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")
load("@rules_python//python:packaging.bzl", "py_wheel")

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

py_wheel(
    name = "hyperon_das_atomdb_wheel",
    distribution = "hyperon_das_atomdb",
    abi = "none",
    python_tag = "py3",
    platform = "any",
    requires_file = "//:requirements_lock.txt",
    stamp = 1,
    version = "0.8.10",
    author = "marcocapozzoli",
    author_email = "marcocapozzoli90@gmail.com",
    summary = "Persistence layer for Distributed AtomSpace",
    description_content_type = "text/markdown",
    description_file = "README.md",
    classifiers = [
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
    ],
    deps = [
        ":hyperon_das_atomdb",
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
