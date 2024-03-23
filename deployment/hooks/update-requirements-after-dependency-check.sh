#!/bin/bash

set -e

pending_update_definition=$(retrieve_json_object_with_property_value "$packages_pending_update" "package_name" "$dependency")

IFS='|' read -r dependency_package_name dependency_current_version dependency_new_version repository_path <<<"$(extract_packages_pending_update_details "$pending_update_definition")"

print "Updating $dependency_package_name to version $dependency_new_version in the $package_name package."

if [ ! -f das-query-engine/requirements.txt ]; then
    print ":red:The das-query-engine/requirements.txt file not found.:/red:"
    exit 1
fi

if grep -q "^${dependency}==" das-query-engine/requirements.txt; then
    sed -i "s/^${dependency}==.*/${dependency}==${dependency_new_version}/" das-query-engine/requirements.txt
    print ":green:Dependency ${dependency} version updated to ${dependency_new_version} in das-query-engine/requirements.txt.:/green:"
else
    echo "${dependency}==${dependency_new_version}" >>das-query-engine/requirements.txt
    print ":green:Dependency ${dependency} added with version ${dependency_new_version} to das-query-engine/requirements.txt.:/green:"
fi
