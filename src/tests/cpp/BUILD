load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_shared_library")

package(default_visibility = ["//visibility:public"])

# Test environment library
cc_library(
    name = "test_environment_lib",
    srcs = [
        "test_environment.cc",
        "test_environment.h",
    ],
    hdrs = ["test_environment.h"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//service_bus:service_bus_lib",
        "@com_github_google_googletest//:gtest",
    ],
)

cc_test(
    name = "shared_queue_test",
    size = "small",
    srcs = ["shared_queue_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//attention_broker:attention_broker_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_test(
    name = "request_selector_test",
    size = "small",
    srcs = ["request_selector_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//attention_broker:attention_broker_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_test(
    name = "attention_broker_test",
    size = "small",
    srcs = [
        "attention_broker_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_test(
    name = "worker_threads_test",
    size = "small",
    srcs = [
        "test_utils.cc",
        "test_utils.h",
        "worker_threads_test.cc",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//attention_broker:attention_broker_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_test(
    name = "handle_trie_test",
    size = "medium",
    srcs = [
        "handle_trie_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//attention_broker:attention_broker_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_test(
    name = "hebbian_network_test",
    size = "medium",
    srcs = [
        "hebbian_network_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_test(
    name = "hebbian_network_updater_test",
    size = "small",
    srcs = [
        "hebbian_network_updater_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//attention_broker:attention_broker_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_test(
    name = "stimulus_spreader_test",
    size = "small",
    srcs = [
        "stimulus_spreader_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//attention_broker:attention_broker_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@mbedtls",
    ],
)

cc_test(
    name = "link_template_test",
    size = "small",
    srcs = [
        "link_template_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//tests/cpp/test_commons",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls",
    ],
)

cc_test(
    name = "nested_link_template_test",
    size = "small",
    srcs = [
        "nested_link_template_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//atomdb:atomdb_singleton",
        "//tests/cpp/test_commons",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls",
    ],
)

cc_test(
    name = "query_answer_test",
    size = "small",
    srcs = [
        "query_answer_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
    ],
    linkstatic = 1,
    deps = [
        "//agents/query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "and_operator_test",
    size = "large",
    srcs = [
        "and_operator_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//agents/query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls",
    ],
)

cc_test(
    name = "unique_assignment_filter_test",
    size = "medium",
    srcs = [
        "test_utils.cc",
        "test_utils.h",
        "unique_assignment_filter_test.cc",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//agents/query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls",
    ],
)

cc_test(
    name = "iterator_test",
    size = "small",
    srcs = [
        "iterator_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//tests/cpp/test_commons",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls",
    ],
)

cc_test(
    name = "query_node_test",
    size = "small",
    srcs = ["query_node_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//agents/query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "leadership_broker_test",
    size = "small",
    srcs = ["leadership_broker_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "message_broker_test",
    size = "small",
    srcs = ["message_broker_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "distributed_algorithm_node_test",
    size = "medium",
    srcs = ["distributed_algorithm_node_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "bus_node_test",
    size = "small",
    srcs = ["bus_node_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//distributed_algorithm_node:bus_node",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "service_bus_test",
    size = "medium",
    srcs = ["service_bus_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//service_bus:service_bus_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "pattern_matching_query_test",
    size = "medium",
    srcs = ["pattern_matching_query_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//agents/query_engine:query_engine_lib",
        "//atomdb:atomdb_singleton",
        "//service_bus:service_bus_lib",
        "//tests/cpp/test_commons",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "context_test",
    size = "small",
    srcs = [
        "context_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//agents/context_broker:context_broker_lib",
        "//agents/query_engine:pattern_matching_query_processor",
        "//atomdb",
        "//attention_broker:attention_broker_lib",
        "//tests/cpp/test_commons",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "atomdb_broker_test",
    size = "small",
    srcs = [
        "atomdb_broker_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//agents/atomdb_broker:atomdb_broker_lib",
        "//atomdb",
        "//attention_broker:attention_broker_lib",
        "//tests/cpp/test_commons",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "query_evolution_test",
    size = "small",
    srcs = ["query_evolution_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//agents/evolution:evolution_lib",
        "//agents/query_engine:pattern_matching_query_processor",
        "//tests/cpp/test_commons",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "link_creation_agent_test",
    size = "small",
    srcs = ["link_creation_agent_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
        "//tests/cpp/test_commons/mocks:mock_atom_db_lib",
        "//tests/cpp/test_commons/mocks:mock_service_bus_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "inference_agent_test",
    size = "small",
    srcs = ["inference_agent_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//agents/inference_agent:inference_agent_lib",
        "//commons:commons_lib",
        "//tests/cpp/test_commons/mocks:mock_atom_db_lib",
        "//tests/cpp/test_commons/mocks:mock_service_bus_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "properties_test",
    size = "small",
    srcs = ["properties_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//atomdb:atomdb_api_types",
        "//commons:commons_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "atom_space_types_test",
    size = "small",
    srcs = ["atom_space_types_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//hasher:hasher_lib",
        "//metta:metta_mapping",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "processor_test",
    size = "small",
    srcs = ["processor_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//commons/processor:processor_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "redis_mongodb_test",
    size = "medium",
    srcs = [
        "redis_mongodb_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//atomdb:atomdb_singleton",
        "//tests/cpp/test_commons",
        "//tests/cpp/test_commons:mock_animals_data_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls",
    ],
)

cc_test(
    name = "redis_mongodb_test_2",
    size = "medium",
    srcs = [
        "redis_mongodb_test_2.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//atomdb:atomdb_singleton",
        "//tests/cpp/test_commons",
        "//tests/cpp/test_commons:mock_animals_data_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls",
    ],
)

cc_test(
    name = "morkdb_test",
    size = "medium",
    srcs = ["morkdb_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkopts = [
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1,
    deps = [
        "//atomdb:atomdb_singleton",
        "//tests/cpp/test_commons",
        "//tests/cpp/test_commons:mock_animals_data_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls",
    ],
)

cc_test(
    name = "inmemorydb_test",
    size = "small",
    srcs = ["inmemorydb_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//atomdb/inmemorydb:inmemorydb_lib",
        "//commons/atoms:atoms_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "profiling_test",
    size = "small",
    srcs = [
        "profiling_test.cc",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
    ],
)

cc_test(
    name = "metta_parser_test",
    size = "small",
    srcs = [
        "metta_parser_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//commons/atoms:atoms_lib",
        "//metta:metta_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)
