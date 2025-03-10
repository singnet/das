package(default_visibility = ["//visibility:public"])

cc_library(
  name = "link_creation_agent_lib",
  srcs = [
    "link_creation_agent.cc",
    "link.cc",
    "service.cc",

  ],
  hdrs = [
    "link_creation_agent.h",
    "link.h",
    "service.h",
  ],
  includes = ["."],
  deps = [
    ":das_agent_node",
    ":link",
    ":link_create_template",
    ":link_creation_agent_node",
    ":service",
    "//distributed_algorithm_node:distributed_algorithm_node_lib",
    "//query_engine:query_engine_lib",
    "//hasher:hasher_lib",
  ],
)

cc_library(
  name = "das_agent_node",
  srcs = ["das_agent_node.cc"],
  hdrs = ["das_agent_node.h", "queue.h"],
  deps = [
    "//query_engine:star_node",
    "//distributed_algorithm_node:distributed_algorithm_node_lib",
  ],
)

cc_library(
  name = "link",
  srcs = ["link.cc"],
  hdrs = ["link.h", "link_create_template.h"],
  deps = [
    "//query_engine:query_engine_lib"
  ],
)

cc_library(
  name = "link_create_template",
  srcs = ["link_create_template.cc"],
  hdrs = ["link_create_template.h"],
  deps = [],
)

cc_library(
  name = "link_creation_agent_node",
  srcs = ["link_creation_agent_node.cc"],
  hdrs = ["link_creation_agent_node.h", "dummy_message.h", "queue.h"],
  deps = [
    "//query_engine:star_node"
  ],
)

cc_library(
  name = "service",
  srcs = ["service.cc"],
  hdrs = ["service.h", "thread_pool.h"],
  deps = [
    ":das_agent_node",
    ":link",
    "//query_engine:query_engine_lib",
  ],
)
