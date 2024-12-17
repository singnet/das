package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hasher_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    includes = ["."],
    copts = [
      "-I/opt/3rd-party/mbedcrypto/include/"
    ],
    deps = [
        #"@mbedcrypto//:lib",
    ]
)
