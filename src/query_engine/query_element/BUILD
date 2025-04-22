package(default_visibility = ["//visibility:public"])

cc_library(
  name = "query_element_lib",
  includes = ["."],
  deps = [
    ":operator",
    ":and",
    ":or",
    ":terminal",
    ":iterator",
    ":sink",
    ":query_element",
    ":link_template",
    ":source",
    ":output_buffers",
    ":query_element_registry",
  ],
)


cc_library(
  name = "operator",
  hdrs = ["Operator.h"],
  deps = [
    ":output_buffers",
    ":query_element",
    "//query_engine:query_answer",
  ],
)

cc_library(
  name = "and",
  hdrs = ["And.h"],
  deps = [
    ":operator",
  ],
)

cc_library(
  name = "or",
  hdrs = ["Or.h"],
  deps = [
    ":operator",
  ],
)

cc_library(
  name = "terminal",
  hdrs = ["Terminal.h"],
  deps = [
    "//atomdb:atomdb_singleton",
    "//hasher:hasher_lib"

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
    "//query_engine:query_node",
    "//commons:commons_lib",
  ],
)


cc_library(
  name = "link_template",
  hdrs = ["LinkTemplate.h"],
  deps = [
    ":and",
    ":or",
    ":iterator",
    ":terminal",
    ":source",
    "//hasher:hasher_lib",
    "//atomdb:atomdb_api_types",
    "//atomdb:atomdb_singleton",
    "//query_engine:query_node",
    "//attention_broker:attention_broker_server_lib",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "query_element_registry",
  hdrs = ["QueryElementRegistry.h"],
  deps = [
    ":query_element",
    "//commons:commons_lib",
  ],
)


cc_library(
  name = "source",
  srcs = ["Source.cc"],
  hdrs = ["Source.h"],
  deps = [
    ":output_buffers",
    ":query_element",
  ],
)

cc_library(
  name = "output_buffers",
  hdrs = ["OutputBuffers.h"],
  deps = [
    "//query_engine:query_node",
  ],
)
