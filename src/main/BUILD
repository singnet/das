load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "attention_broker_main_lib",
    srcs = ["attention_broker_main.cc"],
    deps = [
        "//attention_broker:attention_broker_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
    ],
)

cc_library(
    name = "attention_broker_client_main_lib",
    srcs = ["attention_broker_client_main.cc"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
        "//main/helpers:main_helper_runner_helper_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
    ],
)

cc_library(
    name = "bus_node_lib",
    srcs = ["bus_node.cc"],
    deps = [
        "//agents/atomdb_broker:atomdb_broker_lib",
        "//agents/command_router:command_router_lib",
        "//commons:commons_lib",
        "//main/helpers:main_helper_processor_factory_lib",
    ],
)

cc_library(
    name = "bus_client_lib",
    srcs = ["bus_client.cc"],
    deps = [
        "//agents/command_router:bus_command_router_proxy",
        "//agents/query_engine:query_answer",
        "//atomdb:atomdb_singleton",
        "//atomdb/remotedb:remotedb_lib",
        "//commons:commons_lib",
        "//main/helpers:main_helper_proxy_factory_lib",
        "//service_bus:service_bus_singleton",
    ],
)

cc_library(
    name = "db_loader_main_lib",
    srcs = ["db_loader.cc"],
    deps = [
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
        "//atomdb/adapterdb:adapterdb_lib",
        "//atomdb/morkdb:morkdb_lib",
        "//atomdb/redis_mongodb:redis_mongodb_lib",
        "//atomdb/remotedb:remotedb_lib",
        "//commons:commons_lib",
        "//metta:metta_lib",
    ],
)
