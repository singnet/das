load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_shared_library")
load("@rules_proto_grpc_cpp//:defs.bzl", "cc_grpc_library", "cc_proto_library", "cpp_grpc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lca_integration_test_lib",
    srcs = ["lca_integration_test.cc"],
    deps = [
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
    ],
)
