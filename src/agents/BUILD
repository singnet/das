load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "agents_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":base_proxy",
        "//agents/evolution:evolution_lib",
        "//agents/query_engine:query_engine_lib",
    ],
)

cc_library(
    name = "base_proxy",
    srcs = ["BaseProxy.cc"],
    hdrs = ["BaseProxy.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
    ],
)
