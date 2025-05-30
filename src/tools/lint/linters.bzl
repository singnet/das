load("@aspect_rules_lint//lint:lint_test.bzl", "lint_test")
load("@aspect_rules_lint//lint:ruff.bzl", "lint_ruff_aspect")

ruff = lint_ruff_aspect(
    binary = "@multitool//tools/ruff",
    configs = [
        Label("@//:.ruff.toml"),
    ],
)

ruff_test = lint_test(aspect = ruff)
