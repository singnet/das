load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

cc_grpc_library(
    name = "attention_broker_cc_grpc",
    srcs = [":attention_broker_proto"],
    grpc_only = True,
    visibility = ["//visibility:public"],
    deps = [":attention_broker_cc_proto"],
)

cc_grpc_library(
    name = "atom_space_node_cc_grpc",
    srcs = [":atom_space_node_proto"],
    grpc_only = True,
    visibility = ["//visibility:public"],
    deps = [":atom_space_node_cc_proto"],
)

cc_grpc_library(
    name = "common_cc_grpc",
    srcs = [":common_proto"],
    grpc_only = True,
    visibility = ["//visibility:public"],
    deps = [":common_cc_proto"],
)

cc_proto_library(
    name = "attention_broker_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":attention_broker_proto"],
)

cc_proto_library(
    name = "atom_space_node_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":atom_space_node_proto"],
)

cc_proto_library(
    name = "common_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":common_proto"],
)

proto_library(
    name = "attention_broker_proto",
    srcs = ["attention_broker.proto"],
    visibility = ["//visibility:public"],
    deps = [":common_proto"],
)

proto_library(
    name = "atom_space_node_proto",
    srcs = ["atom_space_node.proto"],
    visibility = ["//visibility:public"],
    deps = [":common_proto"],
)

proto_library(
    name = "common_proto",
    srcs = ["common.proto"],
    visibility = ["//visibility:public"],
)

proto_library(
    name = "echo_proto",
    srcs = ["echo.proto"],
    visibility = ["//visibility:public"],
)
