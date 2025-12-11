load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "processor_lib",
    srcs = [
        "Processor.cc",
        "DedicatedThread.cc",
        "ThreadPool.cc",
    ],
    hdrs = [
        "Processor.h",
        "DedicatedThread.h",
        "ThreadPool.h",
    ],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)
