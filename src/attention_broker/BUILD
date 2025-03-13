package(default_visibility = ["//visibility:public"])

cc_library(
  name = "attention_broker_server_lib",
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
    ":handle_trie",
    ":hebbian_network",
    ":hebbian_network_updater",
    ":request_selector",
    "//commons:commons_lib",
    "//hasher:hasher_lib",
    "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
    "@com_github_grpc_grpc//:grpc++",
    "@com_github_grpc_grpc//:grpc++_reflection",
  ],
)

cc_library(
  name = "handle_trie",
  srcs = ["HandleTrie.cc"],
  hdrs = ["HandleTrie.h"],
  deps = [
    "//commons:commons_lib",
    "//hasher:hasher_lib",
  ]
)

cc_library(
  name = "hebbian_network",
  srcs = ["HebbianNetwork.cc"],
  hdrs = ["HebbianNetwork.h"],
  deps = [
    ":handle_trie",
    "//hasher:hasher_lib",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "hebbian_network_updater",
  srcs = ["HebbianNetworkUpdater.cc"],
  hdrs = ["HebbianNetworkUpdater.h"],
  deps = [
    ":handle_trie",
    ":hebbian_network",
    "//hasher:hasher_lib",
    "//commons:commons_lib",
    "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
    "@com_github_grpc_grpc//:grpc++",
    "@com_github_grpc_grpc//:grpc++_reflection",
  ],
)

cc_library(
  name = "request_selector",
  srcs = ["RequestSelector.cc"],
  hdrs = ["RequestSelector.h"],
  deps = [
    ":hebbian_network",
    "//commons:commons_lib",
  ],
)
