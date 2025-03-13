load("@bazel_skylib//rules:native_binary.bzl", "native_binary")

native_binary(
    name = "clang_tidy",
    src = select(
        {
            "@bazel_tools//src/conditions:linux_x86_64": "@llvm_toolchain_llvm//:bin/clang-tidy",
            "@bazel_tools//src/conditions:linux_aarch64": "@llvm_toolchain_llvm//:bin/clang-tidy",
            "@bazel_tools//src/conditions:darwin_x86_64": "@llvm_toolchain_llvm//:bin/clang-tidy",
            "@bazel_tools//src/conditions:darwin_arm64": "@llvm_toolchain_llvm//:bin/clang-tidy",
        },
    ),
    out = "clang_tidy",
)
