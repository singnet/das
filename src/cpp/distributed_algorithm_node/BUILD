package(default_visibility = ["//visibility:public"])

cc_library(
    name = "distributed_algorithm_node_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    includes = ["."],
    alwayslink = 1,
    linkstatic = 1,
    features = ["fully_static_link"],
    deps = [
        "@com_github_singnet_das_proto//:atom_space_node_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "//commons:commons_lib"
    ],
)
