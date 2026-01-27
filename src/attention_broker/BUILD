load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "attention_broker_lib",
    srcs = [
        "AttentionBrokerServer.cc",
        "StimulusSpreader.cc",
        "WorkerThreads.cc",
    ],
    hdrs = [
        "AttentionBrokerServer.h",
        "StimulusSpreader.h",
        "WorkerThreads.cc",
        "WorkerThreads.h",
    ],
    includes = ["."],
    deps = [
        ":attention_broker_client",
        ":hebbian_network",
        ":hebbian_network_updater",
        ":request_selector",
        "//commons:commons_lib",
        "//commons:handle_trie",
        "//hasher:hasher_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
    ],
)

cc_library(
    name = "attention_broker_client",
    srcs = ["AttentionBrokerClient.cc"],
    hdrs = ["AttentionBrokerClient.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
    ],
)

cc_library(
    name = "hebbian_network",
    srcs = ["HebbianNetwork.cc"],
    hdrs = ["HebbianNetwork.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons:handle_trie",
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "hebbian_network_updater",
    srcs = ["HebbianNetworkUpdater.cc"],
    hdrs = ["HebbianNetworkUpdater.h"],
    includes = ["."],
    deps = [
        ":hebbian_network",
        "//commons:commons_lib",
        "//commons:handle_trie",
        "//hasher:hasher_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
    ],
)

cc_library(
    name = "request_selector",
    srcs = ["RequestSelector.cc"],
    hdrs = ["RequestSelector.h"],
    includes = ["."],
    deps = [
        ":hebbian_network",
        "//commons:commons_lib",
    ],
)
