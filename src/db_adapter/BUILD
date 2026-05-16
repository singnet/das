load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "db_adapter_lib",
    includes = ["."],
    deps = [
        ":atom_persister",
        ":bounded_shared_queue",
        ":context_loader",
        ":database_connection",
        ":database_mapper",
        ":database_orchestrator",
        ":database_types",
        ":database_wrapper",
        ":metta_file_writer",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//db_adapter/mork:mork_lib",
        "//db_adapter/postgres:postgres_lib",
    ],
)

cc_library(
    name = "database_types",
    hdrs = ["DatabaseTypes.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "database_mapper",
    srcs = ["DatabaseMapper.cc"],
    hdrs = ["DatabaseMapper.h"],
    includes = ["."],
    deps = [
        ":database_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//metta:metta_lib",
    ],
)

cc_library(
    name = "database_wrapper",
    srcs = ["DatabaseWrapper.cc"],
    hdrs = ["DatabaseWrapper.h"],
    includes = ["."],
    deps = [
        ":database_connection",
        ":database_mapper",
        ":database_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "context_loader",
    srcs = ["ContextLoader.cc"],
    hdrs = ["ContextLoader.h"],
    includes = ["."],
    deps = [
        ":database_types",
        "//commons:commons_lib",
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "database_connection",
    srcs = ["DatabaseConnection.cc"],
    hdrs = ["DatabaseConnection.h"],
    includes = ["."],
    deps = [
        ":database_types",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//commons/processor:processor_lib",
    ],
)

cc_library(
    name = "database_orchestrator",
    srcs = ["DatabaseOrchestrator.cc"],
    hdrs = ["DatabaseOrchestrator.h"],
    includes = ["."],
    deps = [
        ":bounded_shared_queue",
        ":context_loader",
        ":database_types",
        ":database_wrapper",
        ":metta_file_writer",
        "//atomdb",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//db_adapter/mork:mork_lib",
        "//db_adapter/postgres:postgres_lib",
    ],
)

cc_library(
    name = "atom_persister",
    srcs = ["AtomPersister.cc"],
    hdrs = ["AtomPersister.h"],
    includes = ["."],
    deps = [
        ":bounded_shared_queue",
        ":context_loader",
        ":database_types",
        ":database_wrapper",
        ":metta_file_writer",
        "//atomdb",
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
    ],
)

cc_library(
    name = "bounded_shared_queue",
    srcs = ["BoundedSharedQueue.cc"],
    hdrs = ["BoundedSharedQueue.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)

cc_library(
    name = "metta_file_writer",
    srcs = ["MettaFileWriter.cc"],
    hdrs = ["MettaFileWriter.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
    ],
)
