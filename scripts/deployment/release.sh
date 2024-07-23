#!/bin/bash

set -e

# PATHS
workdir=$(pwd)
hooks_path="$workdir/scripts/deployment/hooks"
definitions_path="$workdir/scripts/deployment/definitions.json"
github_token_path="$workdir/scripts/deployment/gh_token"

source "$workdir/scripts/deployment/utils.sh"

# GLOBAL VARIABLES
required_commands=(git jq curl)
packages_pending_update=""
github_token=$(load_or_request_github_token "$github_token_path")
definitions=$(jq -c '.' "$definitions_path")

function verify_dependencies_updated() {
    local required_dependencies="$1"

    for dependency in $required_dependencies; do
        local is_dependency_updated=$(contains_property_value "$packages_pending_update" "package_name" "$dependency")

        if [ "$is_dependency_updated" -eq 0 ]; then
            if ! boolean_prompt "Dependency $dependency has not been updated prior to the current package. Would you like to proceed regardless? [yes/no] "; then
                print ":yellow:Exiting due to user choice to not proceed without updating dependency...:/yellow:"
                return 1
            fi
        else
            execute_hook_if_exists "AfterDependencyCheck" "$package_hook_after_dependency_check"
        fi
    done
}

function process_package_dependencies() {
    local package_name="$1"
    local required_dependencies="$2"

    if [ "$required_dependencies" = "null" ] || [ -z "$required_dependencies" ] || [ "${#required_dependencies[@]}" -eq 0 ]; then
        print ":yellow:Skipping dependency update for package $package_name as it has no dependent packages.:/yellow:"
        return
    fi

    if ! verify_dependencies_updated "$required_dependencies"; then
        return 1
    fi
}

function execute_hook_if_exists() {
    local hook_name="$1"
    local hook_script="$2"

    if [ -z "$2" ] || [ "$2" == "null" ]; then
        print ":yellow:Skipping hook $hook_name because it could not be loaded; an empty script path was provided.:/yellow:"
        return
    fi

    local script_path="$hooks_path/$hook_script"

    verify_file_exists $script_path

    print "Executing script located at: $script_path"

    source "$script_path"
}

function prompt_for_new_version() {
    local prompt="$1"
    local new_version=""

    while true; do
        new_version=$(text_prompt "$prompt")

        if [[ $(fmt_version "$new_version") -eq 1 ]]; then
            break
        else
            print ":red:The version '$new_version' you provided is invalid. Please use a valid semantic version (e.g., 1.0.0).:/red:"
        fi
    done

    echo "${new_version}"
}

function add_package_to_pending_updates() {
    local new_package="$1"

    if [[ -z "$packages_pending_update" || "$packages_pending_update" == "null" ]]; then
        packages_pending_update="[]"
    fi

    packages_pending_update=$(jq --argjson newPackage "$new_package" '. + [$newPackage]' <<<"$packages_pending_update")
}

function extract_package_details() {
    local package="$1"
    jq -r '[.name, .repository, (.dependencies | join(" ")), .workflow, .hooks.beforeRelease, .hooks.afterDependencyCheck, .ref] | @tsv' <<<"$package" | tr '\t' '|'
}

function extract_packages_pending_update_details() {
    local package="$1"
    jq -r '[.package_name, .current_version, .new_version, .repository_path, .repository_name, .repository_owner, .repository_workflow, .repository_ref] | @tsv' <<<"$package" | tr '\t' '|'
}

function create_release() {
    local package_name="$1"
    local package_repository="$2"
    local package_dependencies="$3"
    local package_workflow="$4"
    local package_hook_before_release="$5"
    local package_repo_owner="$6"
    local package_repo_name="$7"
    local package_repo_ref="$8"

    local package_repository_folder=$(clone_repo_to_temp_dir "$package_repository" "$package_repo_ref")

    cd "$package_repository_folder"

    local package_version=$(get_repository_latest_version)

    local new_package_version=$(prompt_for_new_version "The current version is :green:$package_version:/green:. What is the next version you want to release? (0.0.0): ")

    if process_package_dependencies "$package_name" "$package_dependencies"; then
        execute_hook_if_exists "BeforeRelease" "$package_hook_before_release"

        cd - &>/dev/null

        local new_package="{\"package_name\": \"${package_name}\", \"current_version\": \"${package_version}\", \"new_version\": \"${new_package_version}\", \"repository_path\": \"${package_repository_folder}\", \"repository_owner\": \"${package_repo_owner}\", \"repository_name\": \"${package_repo_name}\", \"repository_workflow\": \"${package_workflow}\", \"repository_ref\": \"$package_repo_ref\"}"

        add_package_to_pending_updates "$new_package"
    fi

}

function prepare_and_release_package() {
    local package_definition="$1"

    IFS='|' read -r package_name package_repository package_dependencies package_workflow package_hook_before_release package_hook_after_dependency_check package_repo_ref <<<"$(extract_package_details "$package_definition")"

    validate_repository_url "$package_repository"

    local package_repo_owner="${BASH_REMATCH[1]}"
    local package_repo_name="${BASH_REMATCH[2]}"

    if boolean_prompt "Do you want to create a release for ${package_name}? [y/n] "; then
        create_release "$package_name" "$package_repository" "$package_dependencies" "$package_workflow" "$package_hook_before_release" "$package_repo_owner" "$package_repo_name" "$package_repo_ref"
    else
        print ":yellow:Skipping release for $package_name:/yellow:"
    fi
}

function review_package_updates() {
    echo "-----------------------------------------------------------------------------------"
    jq -r '.[] | "\(.package_name) \(.current_version) \(.new_version)"' <<<"$packages_pending_update" |
        while IFS= read -r line; do
            package_name=$(echo $line | awk '{print $1}')
            current_version=$(echo $line | awk '{print $2}')
            new_version=$(echo $line | awk '{print $3}')

            print ":green:${package_name}:/green: from version :red:${current_version}:/red: to version :green:${new_version}:/green:"
            echo "-----------------------------------------------------------------------------------"
        done
}

function apply_package_updates() {
    for package in $(echo "$packages_pending_update" | jq -c '.[]'); do
        update_package "$package"
    done
}

function update_package() {
    local package="$1"

    IFS='|' read -r package_name current_version new_version repository_path repository_name repository_owner repository_workflow repository_ref <<<"$(extract_packages_pending_update_details "$package")"

    if [[ -d "$repository_path" ]]; then
        local workflow_inputs='{"version": "'"$new_version"'"}'

        cd "$repository_path"
        show_git_diff
        commit_and_push_changes "Chores: create a new release version $new_version" $repository_ref
        trigger_package_workflow "$package_name" "$workflow_inputs" "$repository_owner" "$repository_name" "$repository_workflow" "$repository_ref"
        cd - &>/dev/null
        print ":green:Package $package_name updated successfully:/green:"
    else
        print ":red:Repository path $repository_path does not exist. Skipping $package_name.:/red:"
    fi
}

function process_package_definitions() {
    local definitions="$1"

    for package_definition in $(jq -c '.[]' <<<"$definitions"); do
        prepare_and_release_package "$package_definition"
    done
}

function review_and_apply_updates() {
    if [ ! -z "$packages_pending_update" ]; then
        review_package_updates

        if boolean_prompt "Do you want to continue and apply these changes? [yes/no] "; then
            if boolean_prompt "Do you want to run the integration tests? [y/n] "; then
                exec_integration_tests=true
            else
                exec_integration_tests=false
            fi

            apply_package_updates
        else
            print ":yellow:You did not continue, so the changes were not applied.:/yellow:"
            exit 0
        fi
    else
        print ":yellow:No pending releases available.:/yellow:"
    fi
}

function main() {
    local exec_integration_tests=false

    requirements "${required_commands[@]}"
    process_package_definitions "$definitions"
    review_and_apply_updates

    if [[ "$exec_integration_tests" == true ]]; then
        source "$workdir/scripts/deployment/tests.sh"
    fi
}

main
