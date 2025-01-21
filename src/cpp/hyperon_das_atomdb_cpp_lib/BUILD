package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hyperon_das_atomdb_cpp_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    copts = [
        "-std=c++20",
        "-fPIC",
        "-Ofast",
    ],
    includes = ["."],
    linkopts = [
        "-lmbedcrypto",
    ],
    linkstatic = True,
    deps = [],
)
