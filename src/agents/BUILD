load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "agents_lib",
    srcs = [],
    hdrs = [],
    includes = ["."],
    deps = [
        ":base_proxy",
        ":base_query_proxy",
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

cc_library(
    name = "base_query_proxy",
    srcs = ["BaseQueryProxy.cc"],
    hdrs = ["BaseQueryProxy.h"],
    includes = ["."],
    deps = [
        ":base_proxy",
        "//agents/query_engine:query_answer",
        "//commons:commons_lib",
        "//atomdb:atomdb_lib",
        "//service_bus:service_bus_lib",
    ],
)
