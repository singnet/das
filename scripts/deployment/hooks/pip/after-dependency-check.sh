#!/bin/bash

set -e

pending_update_definition=$(retrieve_json_object_with_property_value "$packages_pending_update" "package_name" "$dependency")

IFS='|' read -r dependency_package_name dependency_current_version dependency_new_version repository_path <<<"$(extract_packages_pending_update_details "$pending_update_definition")"

print "Updating $dependency_package_name to version $dependency_new_version in the $package_name package."

requirements=($(find . -name requirements.txt))

for requirement_file in $requirements; do
    if grep -q "^${dependency}==" $requirement_file; then
        sed_inplace "s/^${dependency}==.*/${dependency}==${dependency_new_version}/" $requirement_file
        print ":green:Dependency ${dependency} version updated to ${dependency_new_version} in $requirement_file.:/green:"
    else
        print ":yellow:Dependency ${dependency} not found in file $requirement_file:/yellow:"
    fi
done
