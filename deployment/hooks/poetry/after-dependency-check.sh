#!/bin/bash

set -e

pending_update_definition=$(retrieve_json_object_with_property_value "$packages_pending_update" "package_name" "$dependency")

IFS='|' read -r dependency_package_name dependency_current_version dependency_new_version repository_path <<<"$(extract_packages_pending_update_details "$pending_update_definition")"

print "Updating $dependency_package_name to version $dependency_new_version in the $package_name package."

if [ ! -f pyproject.toml ]; then
    print ":red:The pyproject.toml file not found.:/red:"
    exit 1
fi

if grep -q "^${dependency} =" pyproject.toml; then
    sed -i "s/^${dependency} = \".*\"/${dependency} = \"${dependency_new_version}\"/" pyproject.toml
    print ":green:Package ${dependency} version updated to ${dependency_new_version} in pyproject.toml file.:/green:"
else
    sed -i "/^\[tool.poetry.dependencies\]/a ${dependency} = \"${dependency_new_version}\"" pyproject.toml
    print ":green:Package ${dependency} added with version ${dependency_new_version} to pyproject.toml file.:/green:"
fi
