load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",
    srcs = glob(["*.py"]),
)

py_library(
    name = "elements",
    srcs = ["elements.py"],
    deps = [],
)

py_library(
    name = "dict_query_tokenizer",
    srcs = ["dict_query_tokenizer.py"],
    deps = [
        ":elements",
    ],
)
