package(default_visibility = ["//visibility:public"])

cc_library(
    name = "attention_broker_main_lib",
    srcs = ["attention_broker_main.cc"],
    deps = [
        "//attention_broker:attention_broker_server_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)

cc_library(
    name = "query_engine_main_lib",
    srcs = ["query_engine_main.cc"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//query_engine:query_engine_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "query_client_main_lib",
    srcs = ["query_client_main.cc"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//query_engine:query_engine_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "link_creation_engine_main_lib",
    srcs = ["link_creation_engine_main.cc"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//query_engine:query_engine_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "word_query_main_lib",
    srcs = ["word_query_main.cc"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//query_engine:query_engine_lib",
        "//service_bus:service_bus_lib",
    ],
)


cc_library(
    name = "link_creation_agent_main_lib",
    srcs = ["link_creation_agent_main.cc"],
    deps = [
        "//link_creation_agent:link_creation_agent_lib",
    ],
)

cc_library(
    name = "link_creation_agent_client_main_lib",
    srcs = ["link_creation_agent_client_main.cc"],
    deps = [
         "//link_creation_agent:link_creation_agent_lib",

    ],
)

cc_library(
    name = "inference_agent_main_lib",
    srcs = ["inference_agent_main.cc"],
    deps = [
         "//inference_agent:inference_agent_lib",

    ],
)


cc_library(
    name = "inference_agent_client_main_lib",
    srcs = ["inference_agent_client_main.cc"],
    deps = [
         "//inference_agent:inference_agent_lib",

    ],
)
