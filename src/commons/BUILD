load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "commons_lib",
    srcs = [
        "MettaMapping.cc",
        "SharedQueue.cc",
        "StoppableThread.cc",
        "Utils.cc",
    ],
    hdrs = [
        "Atom.h",
        "HandleDecoder.h",
        "Logger.h",
        "MettaMapping.h",
        "Properties.h",
        "RequestValidator.h",
        "SharedQueue.h",
        "Stoppable.h",
        "StoppableThread.h",
        "ThreadPool.h",
        "Utils.h",
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
