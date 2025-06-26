load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "commons_lib",
    srcs = [
        "SharedQueue.cc",
        "StoppableThread.cc",
        "Utils.cc",
        "MettaMapping.cc",
    ],
    hdrs = [
        "Logger.h",
        "Properties.h",
        "RequestValidator.h",
        "SharedQueue.h",
        "Stoppable.h",
        "StoppableThread.h",
        "ThreadPool.h",
        "Utils.h",
        "Atom.h",
        "HandleDecoder.h",
        "MettaMapping.h",
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
