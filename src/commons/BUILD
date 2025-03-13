# TODO: merge `utils` and `commons` (https://github.com/singnet/das/issues/181)

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "commons_lib",
    hdrs = ["RequestValidator.h"],
    includes = ["."],
    deps = [
      ":utils",
      ":shared_queue",
    ],
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
