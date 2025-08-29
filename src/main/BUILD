load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "attention_broker_main_lib",
    srcs = ["attention_broker_main.cc"],
    deps = [
        "//attention_broker:attention_broker_lib",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
    ],
)

cc_library(
    name = "attention_broker_client_main_lib",
    srcs = ["attention_broker_client_main.cc"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//attention_broker:attention_broker_lib",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
    ],
)

cc_library(
    name = "tests_db_loader_main_lib",
    srcs = ["tests_db_loader.cc"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//tests/cpp/test_commons",
        "//tests/cpp/test_commons:mock_animals_data_lib",
    ],
)

cc_library(
    name = "query_engine_main_lib",
    srcs = ["query_engine_main.cc"],
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "evolution_engine_main_lib",
    srcs = ["evolution_engine_main.cc"],
    deps = [
        "//agents/evolution:evolution_lib",
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "evolution_client_main_lib",
    srcs = ["evolution_client_main.cc"],
    deps = [
        "//agents/evolution:evolution_lib",
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "query_client_main_lib",
    srcs = ["query_client_main.cc"],
    deps = [
        "//atom_space:atom_space_lib",
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "link_creation_engine_main_lib",
    srcs = ["link_creation_engine_main.cc"],
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "word_query_main_lib",
    srcs = ["word_query_main.cc"],
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//atom_space:atom_space_lib",
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "word_query_evolution_main_lib",
    srcs = ["word_query_evolution_main.cc"],
    deps = [
        "//agents/evolution:evolution_lib",
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "link_creation_agent_main_lib",
    srcs = ["link_creation_agent_main.cc"],
    deps = [
        "//agents/link_creation_agent:link_creation_agent_lib",
    ],
)

cc_library(
    name = "link_creation_agent_client_main_lib",
    srcs = ["link_creation_agent_client_main.cc"],
    deps = [
        "//agents/link_creation_agent:link_creation_agent_lib",
    ],
)

cc_library(
    name = "inference_agent_main_lib",
    srcs = ["inference_agent_main.cc"],
    deps = [
        "//agents/inference_agent:inference_agent_lib",
    ],
)

cc_library(
    name = "inference_agent_client_main_lib",
    srcs = ["inference_agent_client_main.cc"],
    deps = [
        "//agents/inference_agent:inference_agent_lib",
    ],
)
