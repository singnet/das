load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_shared_library")
load("@rules_proto_grpc_cpp//:defs.bzl", "cc_grpc_library", "cc_proto_library", "cpp_grpc_library")

package(default_visibility = ["//visibility:public"])

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
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "word_query_evolution_main_lib",
    srcs = ["word_query_evolution_main.cc"],
    deps = [
        "//agents/context_broker:context_broker_lib",
        "//agents/evolution:evolution_lib",
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "implication_query_evolution_main_lib",
    srcs = ["implication_query_evolution_main.cc"],
    deps = [
        "//agents/context_broker:context_broker_lib",
        "//agents/evolution:evolution_lib",
        "//agents/query_engine:pattern_matching_query_proxy",
        "//atomdb:atomdb_singleton",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
    ],
)
