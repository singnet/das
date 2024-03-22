#!/bin/bash

set -e

echo "Updating $dependency to version $new_package_version in the $package_name package."

if [ ! -f pyproject.toml ]; then
    echo "The pyproject.toml file not found."
    exit 1
fi

if grep -q "^${dependency} =" pyproject.toml; then
    sed -i "s/^${dependency} = \".*\"/${dependency} = \"${new_package_version}\"/" pyproject.toml
    echo "Package ${dependency} version updated to ${new_package_version} in pyproject.toml."
else
    sed -i "/^\[tool.poetry.dependencies\]/a ${dependency} = \"${new_package_version}\"" pyproject.toml
    echo "Package ${dependency} added with version ${new_package_version} to pyproject.toml."
fi
