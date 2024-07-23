#!/bin/bash

set -e

if ! source "$hooks_path/common/dependency-check.sh"; then
    return 0
fi

requirements=($(find . -name requirements.txt))

for requirement_file in $requirements; do
    if grep -q "^${dependency}==" $requirement_file; then
        sed_inplace "s/^${dependency}==.*/${dependency}==${dependency_new_version}/" $requirement_file
        print ":green:Dependency ${dependency} version updated to ${dependency_new_version} in $requirement_file.:/green:"
    else
        print ":yellow:Dependency ${dependency} not found in file $requirement_file:/yellow:"
    fi
done
