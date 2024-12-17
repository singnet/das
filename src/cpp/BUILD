cc_binary(
    name = "attention_broker_service",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//cpp/main:attention_broker_main_lib",
        "//cpp/utils:utils_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_binary(
    name = "query_broker",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//cpp/main:query_engine_main_lib",
        "//cpp/utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_binary(
    name = "query",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//cpp/main:query_client_main_lib",
        "//cpp/utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_binary(
    name = "link_creation_engine",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//cpp/main:link_creation_engine_main_lib",
        "//cpp/utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_binary(
    name = "word_query",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//cpp/main:word_query_main_lib",
        "//cpp/utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)
