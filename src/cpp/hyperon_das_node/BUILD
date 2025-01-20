load("@nanobind_bazel//:build_defs.bzl", "nanobind_extension")
load("@rules_python//python:packaging.bzl", "py_wheel")

py_library(
    name = "hyperon_das_node",
    srcs = ["__init__.py"],
    data = [":hyperon_das_node_ext"],
    visibility = ["//visibility:public"],
)

nanobind_extension(
    name = "hyperon_das_node_ext",
    srcs = ["hyperon_das_node_ext.cc"],
    deps = ["//distributed_algorithm_node:distributed_algorithm_node_lib"],
)

py_wheel(
    name = "hyperon_das_node_wheel",
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
    description_file = "README_hyperon_das_node.md",
    distribution = "hyperon_das_node",
    platform = "manylinux_2_28_x86_64",
    python_requires = ">=3.10",
    python_tag = "cp310",
    stamp = 1,
    summary = "DAS Node python package",
    version = "$(DAS_NODE_VERSION)",  # must be defined when calling `bazel build` with `--define=DAS_NODE_VERSION=<version>`
    deps = [
        ":hyperon_das_node",
        ":hyperon_das_node_ext",
    ],
)
