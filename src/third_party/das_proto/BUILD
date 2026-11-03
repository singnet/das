load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_proto_grpc_cpp//:defs.bzl", "cc_proto_library", "cc_grpc_library")

package(default_visibility = ["//visibility:public"])

cc_grpc_library(
    name = "attention_broker_cc_grpc",
    protos = [":attention_broker_proto"],
    grpc_only = True,
    deps = [":attention_broker_cc_proto"],
)

cc_grpc_library(
    name = "distributed_algorithm_node_cc_grpc",
    protos = [":distributed_algorithm_node_proto"],
    grpc_only = True,
    deps = [":distributed_algorithm_node_cc_proto"],
)

cc_grpc_library(
    name = "common_cc_grpc",
    protos = [":common_proto"],
    grpc_only = True,
    deps = [":common_cc_proto"],
)

cc_proto_library(
    name = "attention_broker_cc_proto",
    protos = [":attention_broker_proto"],
    deps = [
        ":common_cc_proto"
    ]
)

cc_proto_library(
    name = "distributed_algorithm_node_cc_proto",
    protos = [":distributed_algorithm_node_proto"],
    deps = [
        ":common_cc_proto"
    ]
)

cc_proto_library(
    name = "common_cc_proto",
    protos = [":common_proto"],
)

proto_library(
    name = "attention_broker_proto",
    srcs = ["attention_broker.proto"],
    deps = [":common_proto"],
)

proto_library(
    name = "distributed_algorithm_node_proto",
    srcs = ["distributed_algorithm_node.proto"],
    deps = [":common_proto"],
)

proto_library(
    name = "common_proto",
    srcs = ["common.proto"],
)

proto_library(
    name = "echo_proto",
    srcs = ["echo.proto"],
)
