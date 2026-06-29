load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "command_execution",
    srcs = ["CommandExecution.cc"],
    hdrs = ["CommandExecution.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "command_router_http_api_config",
    srcs = ["CommandRouterHttpAPIConfig.cc"],
    hdrs = ["CommandRouterHttpAPIConfig.h"],
    includes = ["."],
    deps = [
        ":command_execution",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "command_router_http_api",
    srcs = ["CommandRouterHttpAPI.cc"],
    hdrs = ["CommandRouterHttpAPI.h"],
    includes = ["."],
    deps = [
        ":command_execution",
        ":command_router_http_api_config",
        "//commons:commons_lib",
        "//commons/processor:processor_lib",
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "command_router_http_api_singleton",
    srcs = ["CommandRouterHttpAPISingleton.cc"],
    hdrs = ["CommandRouterHttpAPISingleton.h"],
    includes = ["."],
    deps = [
        ":command_router_http_api",
        ":command_router_http_api_config",
        "//commons:commons_lib",
        "//commons/processor:processor_lib",
    ],
)
