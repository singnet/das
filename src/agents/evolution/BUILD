load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "evolution_lib",
    srcs = ["Evolution.cc"],
    hdrs = ["Evolution.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//service_bus:service_bus_lib",
    ],
)
