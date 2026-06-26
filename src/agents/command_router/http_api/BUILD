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
    name = "bus_command_router_proxy_stream_poller",
    srcs = ["BusCommandRouterProxyStreamPoller.cc"],
    hdrs = ["BusCommandRouterProxyStreamPoller.h"],
    includes = ["."],
    deps = [
        "//agents:base_query_proxy",
        "//agents/command_router:bus_command_router_proxy",
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
        ":bus_command_router_proxy_stream_poller",
        ":command_execution",
        ":command_router_http_api_config",
        "//agents/command_router:bus_command_router_proxy",
        "//commons:commons_lib",
        "//commons/processor:processor_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
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
        "//commons/processor:processor_lib",
        "//service_bus:service_bus_lib",
    ],
)
