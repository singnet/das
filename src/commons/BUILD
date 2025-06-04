load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "commons_lib",
    srcs = [
        "SharedQueue.cc",
        "StoppableThread.cc",
        "Utils.cc",
    ],
    hdrs = [
        "Logger.h",
        "RequestValidator.h",
        "SharedQueue.h",
        "Stoppable.h",
        "StoppableThread.h",
        "Utils.h",
        "thread_pool.h"
    ],
    includes = ["."],
    deps = [],
)

cc_library(
    name = "handle_trie",
    srcs = ["HandleTrie.cc"],
    hdrs = ["HandleTrie.h"],
    deps = [
        ":commons_lib",
        "//hasher:hasher_lib",
    ],
)
