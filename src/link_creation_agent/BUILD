package(default_visibility = ["//visibility:public"])

cc_library(
  name = "link_creation_agent_lib",
  srcs = [
    "link_creation_agent.cc",
    "link.cc",
    "link_creation_service.cc",

  ],
  hdrs = [
    "link_creation_agent.h",
    "link.h",
    "link_creation_service.h",
  ],
  includes = ["."],
  deps = [
    ":das_agent_node",
    ":link",
    ":link_create_template",
    ":link_creation_agent_node",
    ":service",
    ":link_creation_db_helper",
    ":processors",
    "//distributed_algorithm_node:distributed_algorithm_node_lib",
    "//agents/query_engine:query_engine_lib",
    "//service_bus:service_bus_lib",
    "//hasher:hasher_lib",
  ],
)

cc_library(
  name = "das_agent_node",
  srcs = ["das_agent_node.cc"],
  hdrs = ["das_agent_node.h", "queue.h"],
  deps = [
    ":link_creation_db_helper",
    "//distributed_algorithm_node:distributed_algorithm_node_lib",
  ],
)

cc_library(
  name = "link",
  srcs = ["link.cc"],
  hdrs = ["link.h", "link_create_template.h"],
  deps = [
    "//agents/query_engine:query_engine_lib"
  ],
)

cc_library(
  name = "link_create_template",
  srcs = ["link_create_template.cc"],
  hdrs = ["link_create_template.h"],
  deps = [
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "link_creation_agent_node",
  srcs = ["link_creation_agent_node.cc"],
  hdrs = ["link_creation_agent_node.h", "dummy_message.h", "queue.h"],
  deps = [
    "//distributed_algorithm_node:star_node"
  ],
)


cc_library(
  name = "link_creation_db_helper",
  srcs = ["link_creation_db_helper.cc"],
  hdrs = ["link_creation_db_helper.h"],
  deps = [
    ":link",
    "//atomdb:atomdb_singleton",
    "//atomdb:atomdb_api_types",
    "//atomdb:atomdb",
  ],
)

cc_library(
  name = "service",
  srcs = ["link_creation_service.cc"],
  hdrs = ["link_creation_service.h", "thread_pool.h"],
  deps = [
    ":das_agent_node",
    ":link",
    ":processors",
    "//agents/query_engine:query_engine_lib",
  ],
)


cc_library(
  name = "processors",
  srcs = ["template_processor.cc", "equivalence_processor.cc", "implication_processor.cc"],
  hdrs = ["link_processor.h", "template_processor.h", "equivalence_processor.h", "implication_processor.h"],
  deps = [
    ":link",
    ":link_create_template",
    ":link_creation_db_helper",
    "//agents/query_engine:query_engine_lib",
    "//commons:commons_lib",
  ],
)
