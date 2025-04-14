package(default_visibility = ["//visibility:public"])

cc_library(
    name = "commons_lib",
    includes = ["."],
    deps = [
        ":logger",
        ":request_validator",
        ":shared_queue",
        ":utils",
    ],
)

cc_library(
    name = "request_validator",
    hdrs = ["RequestValidator.h"],
    deps = [],
)

cc_library(
    name = "utils",
    srcs = ["Utils.cc"],
    hdrs = ["Utils.h"],
    deps = [],
)

cc_library(
    name = "shared_queue",
    srcs = ["SharedQueue.cc"],
    hdrs = ["SharedQueue.h"],
    deps = [],
)

cc_library(
    name = "logger",
    hdrs = ["Logger.h"],
    deps = [],
)
