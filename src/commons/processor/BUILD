load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "processor_lib",
    srcs = [
        "Processor.cc",
    ],
    hdrs = [
        "Processor.h",
    ],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)
