load("@pypi//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "cache",
    srcs = glob(["*.py"]),
    deps = [
        ":attention_broker_gateway",
        ":cache_controller",
        ":iterators",
    ],
)

py_library(
    name = "attention_broker_gateway",
    srcs = ["attention_broker_gateway.py"],
    deps = [
        "//hyperon_das:logger",
        "//hyperon_das:utils",
        "//hyperon_das/grpc:attention_broker_pb2_grpc",
        "//hyperon_das/grpc:attention_broker_pb2",
        "//hyperon_das/grpc:common_pb2",
        requirement("grpcio"),
    ],
)

py_library(
    name = "cache_controller",
    srcs = ["cache_controller.py"],
    deps = [
        ":attention_broker_gateway",
        "//hyperon_das:context",
        "//hyperon_das:utils",
    ],
)

py_library(
    name = "iterators",
    srcs = ["iterators.py"],
    deps = [
        "//hyperon_das:link_filters",
        "//hyperon_das:utils",
        "//hyperon_das/query_engines:query_engine_protocol",
        "//hyperon_das_atomdb",
    ],
)
