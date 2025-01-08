cc_test(
    name = "shared_queue_test",
    srcs = ["shared_queue_test.cc"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "//attention_broker:attention_broker_server_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_test(
    name = "request_selector_test",
    srcs = ["request_selector_test.cc"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "//attention_broker:attention_broker_server_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_test(
    name = "attention_broker_server_test",
    srcs = ["attention_broker_server_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "//attention_broker:attention_broker_server_lib",
        "//utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_test(
    name = "worker_threads_test",
    srcs = ["worker_threads_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "//attention_broker:attention_broker_server_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_test(
    name = "handle_trie_test",
    srcs = ["handle_trie_test.cc", "test_utils.cc", "test_utils.h"],
    size = "medium",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "//attention_broker:attention_broker_server_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_test(
    name = "hebbian_network_test",
    srcs = ["hebbian_network_test.cc", "test_utils.cc", "test_utils.h"],
    size = "medium",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "//attention_broker:attention_broker_server_lib",
        "//utils:utils_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_test(
    name = "hebbian_network_updater_test",
    srcs = ["hebbian_network_updater_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "//attention_broker:attention_broker_server_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_test(
    name = "stimulus_spreader_test",
    srcs = ["stimulus_spreader_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "//attention_broker:attention_broker_server_lib",
        "@mbedcrypto//:lib",
    ],
    linkstatic = 1
)

cc_test(
    name = "link_template_test",
    srcs = ["link_template_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "//query_engine:query_engine_lib",
    ],
    linkopts = [
        #"-L/opt/3rd-party/mbedcrypto",
        "-lmbedcrypto",
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1
)

cc_test(
    name = "nested_link_template_test",
    srcs = ["nested_link_template_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "//query_engine:query_engine_lib",
    ],
    linkopts = [
        "-lmbedcrypto",
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1
)

cc_test(
    name = "query_answer_test",
    srcs = ["query_answer_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "//query_engine:query_engine_lib",
    ],
    linkopts = [
    ],
    linkstatic = 1
)

cc_test(
    name = "and_operator_test",
    srcs = ["and_operator_test.cc", "test_utils.cc", "test_utils.h"],
    size = "medium",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "//query_engine:query_engine_lib",
    ],
    linkopts = [
        #"-L/opt/3rd-party/mbedcrypto",
        "-lmbedcrypto",
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1
)

cc_test(
    name = "iterator_test",
    srcs = ["iterator_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "//query_engine:query_engine_lib",
    ],
    linkopts = [
        #"-L/opt/3rd-party/mbedcrypto",
        "-lmbedcrypto",
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1
)

cc_test(
    name = "remote_sink_iterator_test",
    srcs = ["remote_sink_iterator_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "//query_engine:query_engine_lib",
    ],
    linkopts = [
        "-lmbedcrypto",
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1
)

cc_test(
    name = "das_node_test",
    srcs = ["das_node_test.cc", "test_utils.cc", "test_utils.h"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "//query_engine:query_engine_lib",
    ],
    linkopts = [
        "-lmbedcrypto",
        "-L/usr/local/lib",
        "-lhiredis_cluster",
        "-lhiredis",
        "-lmongocxx",
        "-lbsoncxx",
    ],
    linkstatic = 1
)

cc_test(
    name = "query_node_test",
    srcs = ["query_node_test.cc"],
    size = "small",
    copts = [
      "-Iexternal/gtest/googletest/include",
      "-Iexternal/gtest/googletest",
    ],
    deps = [
        "@com_github_google_googletest//:gtest_main",
        "//query_engine:query_engine_lib",
    ],
    linkstatic = 1
)
