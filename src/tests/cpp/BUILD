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
        "//attention_broker:attention_broker_server_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls//:mbedtls",
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
        "//attention_broker:attention_broker_server_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls//:mbedtls",
    ],
)

cc_test(
    name = "attention_broker_server_test",
    size = "small",
    srcs = [
        "attention_broker_server_test.cc",
        "test_utils.cc",
        "test_utils.h",
    ],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//attention_broker:attention_broker_server_lib",
        "//commons:commons_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls//:mbedtls",
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
        "//attention_broker:attention_broker_server_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls//:mbedtls",
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
        "//attention_broker:attention_broker_server_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls//:mbedtls",
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
        "//attention_broker:attention_broker_server_lib",
        "//commons:commons_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls//:mbedtls",
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
        "//attention_broker:attention_broker_server_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls//:mbedtls",
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
        "//attention_broker:attention_broker_server_lib",
        "@com_github_google_googletest//:gtest_main",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@mbedtls//:mbedtls",
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
        "//query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls//:mbedtls",
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
        "//query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls//:mbedtls",
    ],
)

cc_test(
    name = "handles_answer_test",
    size = "small",
    srcs = [
        "handles_answer_test.cc",
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
        "//query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "count_answer_test",
    size = "small",
    srcs = [
        "count_answer_test.cc",
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
        "//query_engine:query_engine_lib",
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
        "//query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls//:mbedtls",
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
        "//query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls//:mbedtls",
    ],
)

cc_test(
    name = "remote_sink_iterator_test",
    size = "small",
    srcs = [
        "remote_sink_iterator_test.cc",
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
        "//query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls//:mbedtls",
    ],
)

cc_test(
    name = "das_node_test",
    size = "small",
    srcs = [
        "das_node_test.cc",
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
        "//query_engine:query_engine_lib",
        "@com_github_google_googletest//:gtest_main",
        "@mbedtls//:mbedtls",
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
        "//query_engine:query_engine_lib",
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
    name = "link_creation_agent_test",
    size = "medium",
    srcs = ["link_creation_agent_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//link_creation_agent:link_creation_agent_lib",
        "@com_github_google_googletest//:gtest_main",
    ],
)
