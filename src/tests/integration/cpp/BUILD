load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_test(
    name = "inference_integration_test",
    size = "medium",
    srcs = ["inference_integration_test.cc"],
    copts = [
        "-Iexternal/gtest/googletest/include",
        "-Iexternal/gtest/googletest",
    ],
    linkstatic = 1,
    deps = [
        "//tests/integration/cpp/helpers:process_helper",
        "//tests/integration/cpp/helpers:test_helper",
        "@com_github_google_googletest//:gtest_main",
    ],
)
