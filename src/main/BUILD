load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "attention_broker_main_lib",
    srcs = ["attention_broker_main.cc"],
    deps = [
        "//attention_broker:attention_broker_lib",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
    ],
)

cc_library(
    name = "attention_broker_client_main_lib",
    srcs = ["attention_broker_client_main.cc"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//attention_broker:attention_broker_lib",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
    ],
)

cc_library(
    name = "bus_node_lib",
    srcs = ["bus_node.cc"],
    deps = [
        "//agents/atomdb_broker:atomdb_broker_lib",
        "//commons:commons_lib",
        "//main/helpers:main_helper_processor_factory_lib",
    ],
)

cc_library(
    name = "bus_client_lib",
    srcs = ["bus_client.cc"],
    deps = [
        "//commons:commons_lib",
        "//main/helpers:main_helper_proxy_factory_lib",
    ],
)
