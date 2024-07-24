#!/bin/bash

set -e

if [ "$is_dependency_updated" -eq 0 ]; then
    if boolean_prompt "Dependency $dependency has not been updated prior to the current package. Would you like to get the current version from the repo? [yes/no] "; then
        dependency_definition=$(retrieve_json_object_with_property_value "$definitions" "name" "$dependency")

        IFS='|' read -r dependency_package_name package_repository package_workflow package_repo_ref package_hooks <<<"$(extract_package_details "$dependency_definition")"

        local package_repository_folder=$(clone_repo_to_temp_dir "$package_repository" "$package_repo_ref")

        cd "$package_repository_folder"

        local dependency_new_version=$(get_repository_latest_version)

        cd -

        if [ "$dependency_new_version" == "unknown" ]; then
            print ":red: Version could not be detected. Keeping the current version of dependency '$dependency' unchanged. :/red:"
            return 1
        fi

        print "Detected version :green:$dependency_new_version:/green: for dependency ':green:$dependency:green:'."
    else
        print ":yellow:Keeping the current version of dependency '$dependency' unchanged.:/yellow:"
        return 1
    fi
else
    pending_update_definition=$(retrieve_json_object_with_property_value "$packages_pending_update" "package_name" "$dependency")

    IFS='|' read -r dependency_package_name dependency_current_version dependency_new_version repository_path <<<"$(extract_packages_pending_update_details "$pending_update_definition")"
fi

print "Updating $dependency_package_name to version $dependency_new_version in the $package_name package."
