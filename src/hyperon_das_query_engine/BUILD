load("@nanobind_bazel//:build_defs.bzl", "nanobind_extension")
load("@rules_python//python:packaging.bzl", "py_wheel")

py_library(
    name = "hyperon_das_query_engine",
    srcs = ["__init__.py"],
    data = [":hyperon_das_query_engine_ext"],
    visibility = ["//visibility:public"],
)

nanobind_extension(
    name = "hyperon_das_query_engine_ext",
    srcs = ["hyperon_das_query_engine_ext.cc"],
    deps = [
      "//agents/query_engine:query_engine_lib"
    ],
)

py_wheel(
    name="hyperon_das_query_engine_wheel",
    abi="abi3",
    author="Andre Senna",
    author_email="andre@singularitynet.io",
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
    ],
    description_content_type="text/markdown",
    description_file="README_hyperon_das_query_engine.md",
    distribution="hyperon_das_query_engine",
    platform="any",
    python_requires=">=3.10",
    python_tag="cp310",
    stamp=1,
    summary="DAS Node python package",
    version="$(DAS_VERSION)",  # must be defined when calling `bazel build` with `--define=DAS_NODE_VERSION=<version>`
    deps=[
        ":hyperon_das_query_engine",
        ":hyperon_das_query_engine_ext",
    ],
)
