load("@nanobind_bazel//:build_defs.bzl", "nanobind_extension")

py_library(
    name = "hyperon_das_node",
    srcs = ["hyperon_das_node/__init__.py"],
    data = [":hyperon_das_node_ext"],
    visibility = ["//visibility:public"],
)

nanobind_extension(
    name = "hyperon_das_node_ext",
    srcs = ["hyperon_das_node_ext.cc"],
    deps = ["//distributed_algorithm_node:distributed_algorithm_node_lib"],
)

cc_binary(
    name = "attention_broker_service",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:attention_broker_main_lib",
        "//utils:utils_lib",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedcrypto//:lib",
    ],
)

cc_binary(
    name = "query_broker",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:query_engine_main_lib",
        "//utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
)

cc_binary(
    name = "query",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:query_client_main_lib",
        "//utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
)

cc_binary(
    name = "link_creation_engine",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:link_creation_engine_main_lib",
        "//utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
)

cc_binary(
    name = "word_query",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:word_query_main_lib",
        "//utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
)
