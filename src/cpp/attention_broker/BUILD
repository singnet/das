package(default_visibility = ["//visibility:public"])

cc_library(
    name = "attention_broker_server_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    includes = ["."],
    deps = [
        "//utils:utils_lib",
        "//hasher:hasher_lib",
        "@com_github_singnet_das_proto//:attention_broker_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)
