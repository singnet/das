package(default_visibility = ["//visibility:public"])

cc_library(
  name = "service_bus_lib",
  srcs = [],
  hdrs = [],
  includes = ["."],
  deps = [
    ":service_bus_singleton",
    ":service_bus",
    ":bus_command_proxy",
    ":bus_command_processor",
    "//distributed_algorithm_node:distributed_algorithm_node_lib",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "service_bus_singleton",
  srcs = ["ServiceBusSingleton.cc"],
  hdrs = ["ServiceBusSingleton.h"],
  includes = ["."],
  deps = [
    ":service_bus",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "port_pool",
  srcs = ["PortPool.cc"],
  hdrs = ["PortPool.h"],
  includes = ["."],
  deps = [
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "service_bus",
  srcs = ["ServiceBus.cc"],
  hdrs = ["ServiceBus.h"],
  includes = ["."],
  deps = [
    ":bus_command_processor",
    ":port_pool",
    "//distributed_algorithm_node:bus_node",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "bus_command_proxy",
  srcs = ["BusCommandProxy.cc"],
  hdrs = ["BusCommandProxy.h"],
  includes = ["."],
  deps = [
    "//distributed_algorithm_node:bus_node",
    ":port_pool",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "bus_command_processor",
  srcs = ["BusCommandProcessor.cc"],
  hdrs = ["BusCommandProcessor.h"],
  includes = ["."],
  deps = [
    ":bus_command_proxy",
    "//distributed_algorithm_node:bus_node",
    "//commons:commons_lib",
  ],
)
