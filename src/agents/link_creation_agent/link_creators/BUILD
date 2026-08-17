load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# Using glob to avoid necessity of update in this BUILD file when new functions are added
cc_library(
    name = "link_creators_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    includes = ["."],
    deps = [
        "//agents/query_engine:query_answer",
        "//commons:commons_lib",
        "//hasher:hasher_lib",
        "//atomdb:atomdb_singleton",
    ],
)
