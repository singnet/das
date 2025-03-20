package(default_visibility = ["//visibility:public"])

cc_library(
  name = "distributed_algorithm_node_lib",
  includes = ["."],
  alwayslink = 1,
  linkstatic = 1,
  features = ["fully_static_link"],
  deps = [
    ":distributed_algorithm_node",
    ":star_node"
  ]
)

cc_library(
  name = "distributed_algorithm_node",
  srcs = [
    "DistributedAlgorithmNode.cc",
    "LeadershipBroker.cc",
    "MessageBroker.cc",
    "Message.cc",
  ],
  hdrs = [
    "DistributedAlgorithmNode.h",
    "LeadershipBroker.h",
    "MessageBroker.h",
    "Message.h",
  ],
  deps = [
    "@com_github_singnet_das_proto//:atom_space_node_cc_grpc",
    "@com_github_grpc_grpc//:grpc++",
    "@com_github_grpc_grpc//:grpc++_reflection",
    "//commons:commons_lib"
  ],
)


cc_library(
  name = "star_node",
  srcs = ["StarNode.cc"],
  hdrs = ["StarNode.h"],
  includes = ["."],
  deps = [
    "//distributed_algorithm_node:distributed_algorithm_node",
  ],
)
