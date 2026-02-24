load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "commons_lib",
    srcs = [
        "Assignment.cc",
        "SharedQueue.cc",
        "StoppableThread.cc",
        "Utils.cc",
    ],
    hdrs = [
        "Assignment.h",
        "Logger.h",
        "Profiler.h",
        "Properties.h",
        "RequestValidator.h",
        "Serializable.h",
        "SharedQueue.h",
        "Stoppable.h",
        "StoppableThread.h",
        "ThreadPool.h",
        "ThreadSafeHashmap.h",
        "ThreadSafeHeap.h",
        "Utils.h",
    ],
    includes = ["."],
    deps = [
        "//hasher:hasher_lib",
    ],
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
