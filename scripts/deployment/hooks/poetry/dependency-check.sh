#!/bin/bash

set -e

if ! source "$hooks_path/common/dependency-check.sh"; then
    return 0
fi

if [ ! -f pyproject.toml ]; then
    print ":red:The pyproject.toml file not found.:/red:"
    exit 1
fi

if grep -q "^${dependency} =" pyproject.toml; then
    sed_inplace "s/^${dependency} = \".*\"/${dependency} = \"${dependency_new_version}\"/" pyproject.toml
    print ":green:Package ${dependency} version updated to ${dependency_new_version} in pyproject.toml file.:/green:"
else
    sed_inplace "/^\[tool.poetry.dependencies\]/a ${dependency} = \"${dependency_new_version}\"" pyproject.toml
    print ":green:Package ${dependency} added with version ${dependency_new_version} to pyproject.toml file.:/green:"
fi
