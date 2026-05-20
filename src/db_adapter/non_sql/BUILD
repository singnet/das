load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "non_sql_lib",
    includes = ["."],
    deps = [
        ":metta2atoms_mapper",
        "//db_adapter/non_sql/mork:mork_lib",
    ],
)

cc_library(
    name = "metta2atoms_mapper",
    srcs = ["Metta2AtomsMapper.cc"],
    hdrs = ["Metta2AtomsMapper.h"],
    includes = ["."],
    deps = [
        "//commons:commons_lib",
        "//commons/atoms:atoms_lib",
        "//db_adapter:database_mapper",
        "//db_adapter:database_types",
        "//metta:metta_lib",
    ],
)
