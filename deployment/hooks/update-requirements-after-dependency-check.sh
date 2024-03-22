#!/bin/bash

set -e

echo "Updating $dependency to version $new_package_version in the $package_name package."

if [ ! -f das-query-engine/requirements.txt ]; then
    echo "The das-query-engine/requirements.txt file not found."
    exit 1
fi

if grep -q "^${dependency}==" das-query-engine/requirements.txt; then
    sed -i "s/^${dependency}==.*/${dependency}==${new_package_version}/" das-query-engine/requirements.txt
    echo "Dependency ${dependency} version updated to ${new_package_version} in das-query-engine/requirements.txt."
else
    echo "${dependency}==${new_package_version}" >>das-query-engine/requirements.txt
    echo "Dependency ${dependency} added with version ${new_package_version} to das-query-engine/requirements.txt."
fi
