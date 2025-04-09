package(default_visibility = ["//visibility:public"])

cc_library(
    name = "query_element_lib",
    includes = ["."],
    deps = [
        ":and",
        ":iterator",
        ":link_template",
        ":operator",
        ":or",
        ":pattern_matching_cache",
        ":query_element",
        ":remote_iterator",
        ":remote_sink",
        ":sink",
        ":source",
        ":terminal",
    ],
)

cc_library(
    name = "operator",
    hdrs = ["Operator.h"],
    deps = [
        ":query_element",
        "//query_engine:handles_answer",
        "//query_engine:query_answer",
    ],
)

cc_library(
    name = "and",
    hdrs = ["And.h"],
    deps = [
        ":operator",
        "//query_engine:handles_answer",
    ],
)

cc_library(
    name = "or",
    hdrs = ["Or.h"],
    deps = [
        ":operator",
        "//query_engine:handles_answer",
    ],
)

cc_library(
    name = "terminal",
    hdrs = ["Terminal.h"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//hasher:hasher_lib",
    ],
)

cc_library(
    name = "iterator",
    srcs = ["Iterator.cc"],
    hdrs = ["Iterator.h"],
    deps = [
        ":sink",
        "//query_engine:query_answer",
    ],
)

cc_library(
    name = "sink",
    srcs = ["Sink.cc"],
    hdrs = ["Sink.h"],
    deps = [
        ":query_element",
    ],
)

cc_library(
    name = "query_element",
    srcs = ["QueryElement.cc"],
    hdrs = ["QueryElement.h"],
    deps = [
        "//commons:commons_lib",
        "//query_engine:query_node",
    ],
)

cc_library(
    name = "pattern_matching_cache",
    srcs = ["PatternMatchingCache.cc"],
    hdrs = ["PatternMatchingCache.h"],
    deps = [
        "//atomdb:atomdb_singleton",
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "link_template",
    hdrs = ["LinkTemplate.h"],
    deps = [
        ":and",
        ":iterator",
        ":or",
        ":pattern_matching_cache",
        ":source",
        ":terminal",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
        "//attention_broker:attention_broker_server_lib",
        "//commons:commons_lib",
        "//hasher:hasher_lib",
        "//query_engine:handles_answer",
        "//query_engine:query_node",
    ],
)

cc_library(
    name = "remote_iterator",
    srcs = ["RemoteIterator.cc"],
    hdrs = ["RemoteIterator.h"],
    deps = [
        ":query_element",
        "//query_engine:query_answer",
    ],
)

cc_library(
    name = "remote_sink",
    srcs = ["RemoteSink.cc"],
    hdrs = ["RemoteSink.h"],
    deps = [
        ":sink",
        "//commons:commons_lib",
        "//query_engine:query_answer",
        "//query_engine/answer_processor:query_answer_processor",
    ],
)

cc_library(
    name = "source",
    srcs = ["Source.cc"],
    hdrs = ["Source.h"],
    deps = [
        ":query_element",
        "//query_engine:handles_answer",
    ],
)
