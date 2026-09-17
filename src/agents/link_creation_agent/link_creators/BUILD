load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# Using glob to avoid necessity of update in this BUILD file when new functions are added
cc_library(
    name = "link_creators_lib",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    includes = ["."],
    deps = [
        "//agents/query_engine:pattern_matching_query_proxy",
        "//agents/query_engine:query_answer",
        "//atomdb:atomdb_singleton",
        "//attention_broker:attention_broker_lib",
        "//commons:commons_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
    ],
)
