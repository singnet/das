package(default_visibility = ["//visibility:public"])

exports_files(
    [
        ".ruff.toml",
        ".clang-format",
    ],
    visibility = ["//visibility:public"],
)

alias(
    name = "format.check",
    actual = "//tools/format:format.check",
)

alias(
    name = "format",
    actual = "//tools/format",
)

cc_shared_library(
    name = "das",
    additional_linker_inputs = [
    ],
    dynamic_deps = [
    ],
    shared_lib_name = "hyperon_das.so",
    user_link_flags = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    deps = [
        "//agents:agents_lib",
        "//agents/evolution:evolution_lib",
        "//agents/inference_agent:inference_agent_lib",
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//agents/query_engine:query_engine_lib",
        "//atomdb:atomdb_lib",
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//metta:metta_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_binary(
    name = "attention_broker_service",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:attention_broker_main_lib",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls",
    ],
)

cc_binary(
    name = "attention_broker_client",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:attention_broker_client_main_lib",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls",
    ],
)

cc_binary(
    name = "tests_db_loader",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//atomdb:atomdb_lib",
        "//commons:commons_lib",
        "//main:tests_db_loader_main_lib",
    ],
)

cc_binary(
    name = "query_broker",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:query_engine_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "evolution_broker",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:evolution_engine_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "evolution_client",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:evolution_client_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "query",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:query_client_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "link_creation_engine",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:link_creation_engine_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "word_query",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:word_query_main_lib",
        "@mbedtls",
    ],
)

cc_binary(
    name = "word_query_evolution",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//main:word_query_evolution_main_lib",
        "@mbedtls",
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

cc_binary(
    name = "context_broker",
    srcs = [],
    defines = ["BAZEL_BUILD"],
    linkstatic = 1,
    deps = [
        "//main:context_broker_main_lib",
    ],
)
