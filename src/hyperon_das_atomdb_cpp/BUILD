load("@nanobind_bazel//:build_defs.bzl", "nanobind_extension")
load("@rules_python//python:packaging.bzl", "py_wheel")

py_library(
    name = "hyperon_das_atomdb_cpp",
    srcs = glob(["**/*.py"]),
    data = [":ext"],
    visibility = ["//visibility:public"],
)

nanobind_extension(
    name = "ext",
    srcs = ["hyperon_das_atomdb_cpp_bind.cc"] + glob(["*.h"]),
    includes = ["//hyperon_das_atomdb_cpp_lib"],
    deps = ["//hyperon_das_atomdb_cpp_lib"],
)

py_wheel(
    name = "hyperon_das_atomdb_cpp_wheel",
    abi = "abi3",
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
    distribution = "hyperon_das_atomdb_cpp",
    platform = "manylinux_2_28_x86_64",
    python_requires = ">=3.10",
    python_tag = "cp310",
    stamp = 1,
    summary = "Atom Space DB for Hyperon DAS",
    version = "$(ATOMDB_VERSION)",  # must be defined when calling `bazel build` with `--define=ATOMDB_VERSION=<version>`
    deps = [
        ":ext",
        ":hyperon_das_atomdb_cpp",
    ],
)
