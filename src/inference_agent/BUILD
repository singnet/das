package(default_visibility = ["//visibility:public"])

cc_library(
  name = "inference_agent_lib",
  srcs = [
    "inference_agent.cc",
  ],
  hdrs = [
    "inference_agent.h",
    "inference_iterator.h",
    "inference_request_validator.h",
  ],
  includes = ["."],
  deps = [
    ":distributed_inference_control_agent_node",
    ":inference_request",
    ":inference_agent_node",
    "//link_creation_agent:link_creation_agent_lib",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "distributed_inference_control_agent_node",
  srcs = ["distributed_inference_control_agent_node.cc"],
  hdrs = ["distributed_inference_control_agent_node.h"],
  deps = [
    "//query_engine:star_node",
  ],
)

cc_library(
  name = "inference_request",
  srcs = ["inference_request.cc"],
  hdrs = ["inference_request.h"],
  deps = [
    "//commons:commons_lib",
    "//link_creation_agent:link_creation_agent_lib",
  ],
)

cc_library(
  name = "inference_agent_node",
  srcs = ["inference_agent_node.cc"],
  hdrs = ["inference_agent_node.h"],
  deps = [
    "//commons:commons_lib",
    "//link_creation_agent:link_creation_agent_lib",
    "//query_engine:star_node",
    "//distributed_algorithm_node:distributed_algorithm_node_lib"
  ],
)
