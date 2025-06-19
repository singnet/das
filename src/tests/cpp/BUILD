load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

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
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
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
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
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
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
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
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
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
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
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
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
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
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
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
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
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
    size = "small",
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
    size = "medium",  # due to the 3 seconds sleep in the test
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
    linkstatic = 1,
    deps = [
        "//agents/evolution:evolution_lib",
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
        "//atom_space:atom_space_types",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "atom_space_test",
    size = "small",
    srcs = ["atom_space_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//commons:commons_lib",
        "//agents/query_engine:query_engine_lib",
        "//atom_space:atom_space_lib",
        "//atom_space:atom_space_types",
        "//atomdb:atomdb_api_types",
        "//service_bus:service_bus_lib",
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
        "//atom_space:link_template_interface",
        "//atomdb:atomdb_singleton",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls",
    ],
)
