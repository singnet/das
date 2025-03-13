load("@rules_python//python:defs.bzl", "py_library")
load("@pypi//:requirements.bzl", "requirement")


package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",
    srcs = glob(["*.py"]),
)

py_library(
    name = "das_node",
    srcs = ["das_node.py"],
    deps = [
        ":remote_iterator",
        ":star_node",
    ],
)

py_library(
    name = "query_answer",
    srcs = ["query_answer.py"],
    deps = [],
)

py_library(
    name = "query_element",
    srcs = ["query_element.py"],
    deps = [],
)

py_library(
    name = "query_node",
    srcs = ["query_node.py"],
    deps = [
        ":query_answer",
        ":shared_queue"
    ],
)

py_library(
    name = "remote_iterator",
    srcs = ["remote_iterator.py"],
    deps = [
        ":query_answer",
        ":query_element",
        ":query_node",
    ],
)

py_library(
    name = "shared_queue",
    srcs = ["shared_queue.py"],
    deps = [],
)


py_library(
    name = "star_node",
    srcs = ["star_node.py"],
    deps = [],
)






