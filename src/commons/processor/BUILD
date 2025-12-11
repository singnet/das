load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "processor_lib",
    srcs = [
        "DedicatedThread.cc",
        "Processor.cc",
        "ThreadPool.cc",
    ],
    hdrs = [
        "DedicatedThread.h",
        "Processor.h",
        "ThreadPool.h",
    ],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)
