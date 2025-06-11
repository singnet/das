load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "link_creation_agent_lib",
    srcs = [
        "link.cc",
        "link_creation_agent.cc",
        "link_creation_service.cc",
    ],
    hdrs = [
        "link.h",
        "link_creation_agent.h",
        "link_creation_service.h",
    ],
    includes = ["."],
    deps = [
        ":das_agent_node",
        ":link",
        ":link_create_template",
        ":link_creation_agent_node",
        ":link_creation_db_helper",
        ":processors",
        ":service",
        "//agents/query_engine:query_engine_lib",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
        "//hasher:hasher_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "das_agent_node",
    srcs = ["das_agent_node.cc"],
    hdrs = [
        "das_agent_node.h",
        "queue.h",
    ],
    deps = [
        ":link_creation_db_helper",
        "//distributed_algorithm_node:distributed_algorithm_node_lib",
    ],
)

cc_library(
    name = "link",
    srcs = ["link.cc"],
    hdrs = [
        "link.h",
        "link_create_template.h",
    ],
    deps = [
        "//agents/query_engine:query_engine_lib",
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
    hdrs = [
        "dummy_message.h",
        "link_creation_agent_node.h",
        "queue.h",
    ],
    deps = [
        "//distributed_algorithm_node:star_node",
    ],
)

cc_library(
    name = "link_creation_db_helper",
    srcs = ["link_creation_db_helper.cc"],
    hdrs = ["link_creation_db_helper.h"],
    deps = [
        ":link",
        "//atomdb",
        "//atomdb:atomdb_api_types",
        "//atomdb:atomdb_singleton",
    ],
)

cc_library(
    name = "service",
    srcs = ["link_creation_service.cc"],
    hdrs = [
        "link_creation_service.h",
        "thread_pool.h",
    ],
    deps = [
        ":das_agent_node",
        ":link",
        ":processors",
        "//agents/query_engine:query_engine_lib",
    ],
)

cc_library(
    name = "processors",
    srcs = [
        "equivalence_processor.cc",
        "implication_processor.cc",
        "template_processor.cc",
    ],
    hdrs = [
        "equivalence_processor.h",
        "implication_processor.h",
        "link_processor.h",
        "template_processor.h",
    ],
    deps = [
        ":link",
        ":link_create_template",
        ":link_creation_db_helper",
        "//agents/query_engine:query_engine_lib",
        "//commons:commons_lib",
    ],
)
