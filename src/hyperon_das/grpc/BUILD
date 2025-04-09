load("@pypi_das//:requirements.bzl", "requirement")
load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "common_pb2",
    srcs = ["common_pb2.py"],
    data = ["common_pb2.pyi"],  # see: https://github.com/rules-proto-grpc/rules_proto_grpc/issues/257
    deps = [
        requirement("protobuf"),
    ],
)

py_library(
    name = "common_pb2_grpc",
    srcs = ["common_pb2_grpc.py"],
    deps = [
        requirement("grpcio"),
    ],
)

py_library(
    name = "attention_broker_pb2_grpc",
    srcs = ["attention_broker_pb2_grpc.py"],
    deps = [
        ":common_pb2",
        requirement("grpcio"),
    ],
)

py_library(
    name = "attention_broker_pb2",
    srcs = ["attention_broker_pb2.py"],
    data = ["attention_broker_pb2.pyi"],  # see: https://github.com/rules-proto-grpc/rules_proto_grpc/issues/257
    deps = [
        ":common_pb2",
        requirement("protobuf"),
    ],
)
