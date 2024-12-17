package(default_visibility = ["//visibility:public"])

cc_library(
    name = "attention_broker_main_lib",
    srcs = ["attention_broker_main.cc"],
    hdrs = glob(["*.h"]),
    deps = [
        "//cpp/attention_broker:attention_broker_server_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)

cc_library(
    name = "query_engine_main_lib",
    srcs = ["query_engine_main.cc"],
    hdrs = glob(["*.h"]),
    deps = [
        "//cpp/query_engine:query_engine_lib",
    ],
)

cc_library(
    name = "query_client_main_lib",
    srcs = ["query_client_main.cc"],
    hdrs = glob(["*.h"]),
    deps = [
        "//cpp/query_engine:query_engine_lib",
    ],
)

cc_library(
    name = "link_creation_engine_main_lib",
    srcs = ["link_creation_engine_main.cc"],
    hdrs = glob(["*.h"]),
    deps = [
        "//cpp/query_engine:query_engine_lib",
    ],
)

cc_library(
    name = "word_query_main_lib",
    srcs = ["word_query_main.cc"],
    hdrs = glob(["*.h"]),
    deps = [
        "//cpp/query_engine:query_engine_lib",
    ],
)
