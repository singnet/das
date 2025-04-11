load("@rules_python//python:defs.bzl", "py_library")
load("@pypi_das//:requirements.bzl", "requirement")


package(default_visibility = ["//visibility:public"])

filegroup(
    name = "py_files",
    srcs = glob(["*.py"]),
)

filegroup(
    name = "py_all_files",
    srcs = glob(["**/*.py"]),
)


py_binary(
    name = "main",
    srcs = ["main.py"],
    deps = [
      ":optimizer"
    ],
)

py_library(
    name = "attention_broker_updater",
    srcs = ["attention_broker_updater.py"],
    deps = [
        "//hyperon_das_node:hyperon_das_node",
        "//hyperon_das/grpc:attention_broker_pb2_grpc",
        "//hyperon_das/grpc:common_pb2",
        "//hyperon_das/grpc:attention_broker_pb2",
        requirement("grpcio"),
    ],
)

py_library(
    name = "fitness_functions",
    srcs = ["fitness_functions.py"],
    deps = [
      "//hyperon_das_atomdb:exceptions",
    ],
)

py_library(
    name = "selection_methods",
    srcs = ["selection_methods.py"],
    deps = [],
)

py_library(
    name = "utils",
    srcs = ["utils.py"],
    deps = [],
)

py_library(
    name = "node",
    srcs = ["node.py"],
    deps = ["//hyperon_das_node:hyperon_das_node"],
)

py_library(
    name = "optimizer",
    srcs = ["optimizer.py"],
    deps = [
        ":fitness_functions",
        ":selection_methods",
        ":utils",
        ":node",
        ":attention_broker_updater",
        "//hyperon_das:hyperon_das",
        "//hyperon_das_atomdb:hyperon_das_atomdb",
        "//hyperon_das_node:hyperon_das_node"
    ],
)