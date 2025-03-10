package(default_visibility = ["//visibility:public"])

exports_files(
    [
        ".ruff.toml",
    ],
    visibility = ["//visibility:public"],
)

alias(
  name = "format.check",
  actual = "//tools/format:format.check"
)

alias(
  name = "format",
  actual = "//tools/format"
)

cc_binary(
    name = "attention_broker_service",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:attention_broker_main_lib",
        "//commons:commons_lib",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls//:mbedtls",
    ],
)

cc_binary(
    name = "query_broker",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:query_engine_main_lib",
        "//commons:commons_lib",
        "@mbedtls//:mbedtls",
    ],
)

cc_binary(
    name = "query",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:query_client_main_lib",
        "//commons:commons_lib",
        "@mbedtls//:mbedtls",
    ],
)

cc_binary(
    name = "link_creation_engine",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:link_creation_engine_main_lib",
        "//commons:commons_lib",
        "@mbedtls//:mbedtls",
    ],
)

cc_binary(
    name = "word_query",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:word_query_main_lib",
        "//commons:commons_lib",
        "@mbedtls//:mbedtls",
    ],
)



cc_binary(
    name = "link_creation_server",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:link_creation_agent_main_lib",
    ],
)


cc_binary(
    name = "link_creation_agent_client",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:link_creation_agent_client_main_lib",
    ],
)

cc_binary(
    name = "inference_agent_server",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:inference_agent_main_lib",
    ],
)


cc_binary(
    name = "inference_agent_client",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:inference_agent_client_main_lib",
    ],
)
