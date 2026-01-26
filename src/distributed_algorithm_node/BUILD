load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "distributed_algorithm_node_lib",
    features = ["fully_static_link"],
    includes = ["."],
    linkstatic = 1,
    deps = [
        ":bus_node",
        ":distributed_algorithm_node",
        ":star_node",
    ],
    alwayslink = 1,
)

cc_library(
    name = "distributed_algorithm_node",
    srcs = [
        "DistributedAlgorithmNode.cc",
        "LeadershipBroker.cc",
        "Message.cc",
        "MessageBroker.cc",
    ],
    hdrs = [
        "DistributedAlgorithmNode.h",
        "LeadershipBroker.h",
        "Message.h",
        "MessageBroker.h",
    ],
    deps = [
        "//commons:commons_lib",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:distributed_algorithm_node_cc_grpc",
    ],
)

cc_library(
    name = "star_node",
    srcs = ["StarNode.cc"],
    hdrs = ["StarNode.h"],
    includes = ["."],
    deps = [
        "//distributed_algorithm_node",
    ],
)

cc_library(
    name = "bus_node",
    srcs = ["BusNode.cc"],
    hdrs = ["BusNode.h"],
    includes = ["."],
    deps = [
        "//distributed_algorithm_node",
    ],
)
