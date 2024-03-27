#!/bin/bash

set -e

local pending_update_definition=$(retrieve_json_object_with_property_value "$packages_pending_update" "package_name" "$dependency")
local config_file_path="src/config/config.py"

IFS='|' read -r dependency_package_name dependency_current_version dependency_new_version repository_path <<<"$(extract_packages_pending_update_details "$pending_update_definition")"

print "Updating $dependency_package_name to version $dependency_new_version in the $package_name package."

if [ ! -f "$config_file_path" ]; then
    print ":red:The $config_file_path file not found.:/red:"
    exit 1
fi

if [ "$dependency_package_name" == "das-metta-parser" ]; then
    if grep -q "^METTA_PARSER_IMAGE_VERSION =" $config_file_path; then
        sed -i "s/^METTA_PARSER_IMAGE_VERSION = \".*\"/METTA_PARSER_IMAGE_VERSION = \"${dependency_new_version}-metta-parser\"/" "$config_file_path"
        print ":green:Package ${dependency_package_name} version updated to ${dependency_new_version} in $config_file_path file.:/green:"
    else
        print ":red:Variable 'METTA_PARSER_IMAGE_VERSION' not found in the $config_file_path file.:/red:"
    fi
else
    print ":yellow:Dependency $dependency_package_name action not set up in the after-dependency-check script:/yellow:"
fi
