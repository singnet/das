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
    name = "proxy_parameters_from_json",
    srcs = ["ProxyParametersFromJson.cc"],
    hdrs = ["ProxyParametersFromJson.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "http_command_proxy_factory",
    srcs = ["HttpCommandProxyFactory.cc"],
    hdrs = ["HttpCommandProxyFactory.h"],
    includes = ["."],
    deps = [
        ":proxy_parameters_from_json",
        "//agents/command_router:bus_command_router_proxy",
        "//commons:commons_lib",
        "@nlohmann_json//:json",
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
        ":http_command_proxy_factory",
        "//agents/command_router:bus_command_router_processor",
        "//agents/command_router:bus_command_router_proxy",
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
        "//agents/command_router:bus_command_router_processor",
        "//commons/processor:processor_lib",
        "//service_bus:bus_command_processor",
    ],
)
