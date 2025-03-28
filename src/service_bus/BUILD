package(default_visibility = ["//visibility:public"])

cc_library(
  name = "service_bus_lib",
  srcs = [],
  hdrs = [],
  includes = ["."],
  deps = [
    ":service_bus_singleton",
    ":service_bus",
    ":bus_command_processor",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "service_bus_singleton",
  srcs = ["ServiceBusSingleton.cc"],
  hdrs = ["ServiceBusSingleton.h"],
  deps = [
    ":service_bus",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "service_bus",
  srcs = ["ServiceBus.cc"],
  hdrs = ["ServiceBus.h"],
  deps = [
    ":bus_command_processor",
    "//distributed_algorithm_node:bus_node",
    "//commons:commons_lib",
  ],
)

cc_library(
  name = "bus_command_processor",
  srcs = ["BusCommandProcessor.cc"],
  hdrs = ["BusCommandProcessor.h"],
  deps = [
    "//distributed_algorithm_node:bus_node",
    "//commons:commons_lib",
  ],
)
