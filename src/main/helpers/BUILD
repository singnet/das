load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "main_helper_processor_factory_lib",
    hdrs = [
        "ProcessorFactory.h",
    ],
    includes = ["."],
    deps = [
        "//agents/atomdb_broker:atomdb_broker_lib",
        "//agents/context_broker:context_broker_lib",
        "//agents/evolution:evolution_lib",
        "//agents/inference_agent:inference_agent_lib",
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//agents/query_engine:query_engine_lib",
        "//commons:commons_lib",
        "//main/helpers:main_helper_runner_helper_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "main_helper_proxy_factory_lib",
    hdrs = [
        "ProxyFactory.h",
    ],
    includes = ["."],
    deps = [
        "//agents/atomdb_broker:atomdb_broker_lib",
        "//agents/context_broker:context_broker_lib",
        "//agents/evolution:evolution_lib",
        "//agents/inference_agent:inference_agent_lib",
        "//agents/link_creation_agent:link_creation_agent_lib",
        "//agents/query_engine:query_engine_lib",
        "//commons:commons_lib",
        "//main/helpers:main_helper_runner_helper_lib",
        "//service_bus:service_bus_lib",
    ],
)

cc_library(
    name = "main_helper_runner_helper_lib",
    srcs = [
        "RunnerHelper.cc",
    ],
    hdrs = [
        "RunnerHelper.h",
    ],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)
