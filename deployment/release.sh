#!/bin/bash

set -e

workdir=$(pwd)
hooks_path="$workdir/deployment/hooks"
definitions_path="$workdir/deployment/definitions.json"
secret_path="$workdir/deployment/secret"
packages_pending_update="[]"

function boolean_prompt() {
    local prompt="$1"

    while true; do
        read -p "$prompt" answer
        answer=$(echo "$answer" | tr '[:upper:]' '[:lower:]')

        case "${answer}" in
        "y" | "yes")
            echo 1
            return
            ;;
        "n" | "no")
            echo 0
            return
            ;;
        *)
            continue
            ;;
        esac
    done
}

function text_prompt() {
    local prompt="$1"

    read -p "$prompt" answer

    echo $answer
}

function get_repository_latest_version() {
    local latest_version=$(git for-each-ref --sort=-taggerdate --format '%(refname:short)' refs/tags |
        grep -E '^(v?[0-9]+\.[0-9]+\.[0-9]+)$' |
        head -n 1)

    if [ -z $latest_version ]; then
        echo "unknown"
    fi

    echo $latest_version
}

function retrieve_json_object_with_property_value() {
    local json_list="$1"
    local target_property="$2"
    local target_value="$3"

    for item in $(echo "$json_list" | jq -c '.[]'); do
        local property_value=$(echo "$item" | jq -r ".$target_property")

        if [ "$property_value" == "$target_value" ]; then
            echo "$item"
            return
        fi
    done
}

function contains_property_value() {
    local json_list="$1"
    local target_property="$2"
    local target_value="$3"

    local item=$(retrieve_json_object_with_property_value "$json_list" "$target_property" "$target_value")

    if [ ! -z "$item" ]; then
        echo 1
    else
        echo 0
    fi
}

function dependency_check() {
    local package_dependencies="$1"

    for dependency in $package_dependencies; do
        dependency_updated=$(contains_property_value "$packages_pending_update" "package_name" "$dependency")

        if [ "$dependency_updated" -eq 0 ]; then
            local continue_without_dependency_update=$(boolean_prompt "Dependency $dependency has not been updated prior to the current package. Would you like to proceed regardless? [yes/no] ")

            if [ "$continue_without_dependency_update" -eq 0 ]; then
                echo "Exiting due to user interruption..."
                exit 0
            fi
        else
            run_hook "AfterDependencyCheck" "$package_hook_after_dependency_check"
        fi
    done
}

function update_package_dependencies() {
    local package_dependencies=$1
    local package_name=$(jq -r '.name' <<<"$package_definition")

    if [ "$package_dependencies" = "null" ] || [ -z "$package_dependencies" ] || [ "${#package_dependencies[@]}" -eq 0 ]; then
        echo "Skipping dependency update for package $package_name as it has no dependent packages."
        return
    fi

    dependency_check $package_dependencies
}

function fmt_version() {
    local version="$1"
    local pattern="^[0-9]+\.[0-9]+\.[0-9]+$"

    if [[ "$version" =~ $pattern ]]; then
        echo 1
        return
    fi

    echo 0
    return
}

function run_hook() {
    local hook_name="$1"
    local hook_script="$2"

    if [ -z "$2" ] || [ "$2" == "null" ]; then
        echo "Skipping hook $hook_name because it could not be loaded; an empty script path was provided."
        return
    fi

    local script_path="$hooks_path/$hook_script"

    echo "Executing script located at: $script_path"

    source "$script_path"
}

function make_tag() {
    local new_package_version=""

    local latest_package_version=$(get_repository_latest_version)

    while true; do
        new_package_version=$(text_prompt "The $package_name current version is $latest_package_version. What is the next version you want to release? (0.0.0) ")

        is_valid_version=$(fmt_version $new_package_version)

        if [ "$is_valid_version" -eq 0 ]; then
            echo "The version '$new_package_version' you provided is invalid"
            continue
        fi

        local versions=("$latest_package_version" "$new_package_version")

        echo "${versions[@]}"

        return
    done
}

function clone_repository() {
    local package_repository=$1
    local tmp_folder=$(mktemp -d)

    git clone $package_repository $tmp_folder &>/dev/null

    echo $tmp_folder
}

function push_to_packages_pending_update() {
    local new_package="$1"

    packages_pending_update=$(echo "$packages_pending_update" | jq ". + [$new_package]")
}

function make_release() {
    local package_definition="$1"

    local package_name=$(jq -r '.name' <<<"$package_definition")
    local package_repository=$(jq -r '.repository' <<<"$package_definition")
    local package_dependencies=$(jq -r '.dependencies[]' <<<"$package_definition")
    local package_workflow=$(jq -r '.workflow' <<<"$package_definition")

    if [[ $package_repository =~ git@github.com:([^/]+)/([^/.]+)\.git ]]; then
        package_repo_owner="${BASH_REMATCH[1]}"
        package_repo_name="${BASH_REMATCH[2]}"
    else
        echo "Invalid format for $package_name repository SSH URL in definitions.json"
        exit 1
    fi

    package_hook_before_release=$(jq -r '.hooks.beforeRelease' <<<"$package_definition")
    package_hook_after_dependency_check=$(jq -r '.hooks.afterDependencyCheck' <<<"$package_definition")

    local create_release=$(boolean_prompt "Do you want to create a release for ${package_name}: [yes/no] ")

    if [ "$create_release" -eq 1 ]; then
        local package_repository_folder=$(clone_repository $package_repository)

        cd $package_repository_folder

        update_package_dependencies $package_dependencies

        local versions=($(make_tag $package_repository_folder))
        package_version="${versions[0]}"
        new_package_version="${versions[1]}"

        run_hook "BeforeRelease" "$package_hook_before_release"

        cd - &>/dev/null

        local new_package="{\"package_name\": \"${package_name}\", \"current_version\": \"${package_version}\", \"new_version\": \"${new_package_version}\", \"repository_path\": \"${package_repository_folder}\", \"repository_owner\": \"${package_repo_owner}\", \"repository_name\": \"${package_repo_name}\", \"repository_workflow\": \"${package_workflow}\"}"

        push_to_packages_pending_update "$new_package"
    else
        echo "Skipping release for $package_name"
    fi
}

function commit_changes() {
    commit_msg="$1"

    git_status=$(git status --porcelain)

    if [ -n "$git_status" ]; then
        setup_git
        git add .
        git commit -m "$commit_msg"
        git push origin main
    else
        echo "Skipping commit changes because no files were changed to be committed"
    fi

}

function setup_git() {
    # git_user=$(git config user.name)
    # git_email=$(git config user.email)
    while [ -z "$git_user" ] || [ -z "$git_email" ]; do
        git_user=$(text_prompt "Your Git username is not configured. Please provide your Git name: ")
        git_email=$(text_prompt "Your Git email is not configured. Please provide your Git email: ")
    done

    git config user.name "$git_user"
    git config user.email "$git_email"
}

function review() {
    for package in $(jq -c '.[]' <<<"$packages_pending_update"); do
        local package_name=$(jq -r '.package_name' <<<"$package")
        local package_current_version=$(jq -r '.current_version' <<<"$package")
        local package_new_version=$(jq -r '.new_version' <<<"$package")

        echo "$package_name from version $package_current_version to version $package_new_version"
    done
}

function start_workflow() {
    new_version="$1"
    repository_owner="$2"
    repository_name="$3"
    repository_workflow="$4"

    api_url="https://api.github.com/repos/$repository_owner/$repository_name/actions/workflows/$repository_workflow/dispatches"

    curl -X POST \
        -H "Authorization: token $github_token" \
        -H "Accept: application/vnd.github.v3+json" \
        "$api_url" \
        -d '{"ref": "main", "inputs": {"version": "'"$new_version"'"}}'

    if [ $? -ne 0 ]; then
        echo "Failed to trigger workflow"
        exit 1
    fi

    sleep 10

    while true; do
        status_response=$(curl -s -X GET \
            -H "Authorization: token $github_token" \
            -H "Accept: application/vnd.github.v3+json" \
            "https://api.github.com/repos/$repository_owner/$repository_name/actions/workflows/$repository_workflow/runs?event=workflow_dispatch")

        repository_workflow_path=".github/workflows/$repository_workflow"

        status=$(echo "$status_response" | jq -r '.workflow_runs[] | select(.path == "'"$repository_workflow_path"'") | .status' | head -n 1)

        if [ -n "$status" ]; then
            if [ "$status" == "completed" ]; then
                echo "Workflow completed successfully."
                break
            elif [ "$status" == "failed" ]; then
                echo "Workflow failed."
                exit 1
            else
                echo "Workflow is currently in the '$status' status. Please wait while it completes..."
                sleep 30
            fi
        else
            echo "No matching or active workflow runs found. Exiting monitoring loop."
            break
        fi

        sleep 10
    done

}

function update_packages() {
    for package in $(echo "$packages_pending_update" | jq -c '.[]'); do
        local package_name=$(echo "$package" | jq -r '.package_name')
        local current_version=$(echo "$package" | jq -r '.current_version')
        local new_version=$(echo "$package" | jq -r '.new_version')
        local repository_path=$(echo "$package" | jq -r '.repository_path')
        local repository_name=$(echo "$package" | jq -r '.repository_name')
        local repository_owner=$(echo "$package" | jq -r '.repository_owner')
        local repository_workflow=$(echo "$package" | jq -r '.repository_workflow')

        cd $repository_path

        commit_changes "chores: create a new release version"
        start_workflow $new_version $repository_owner $repository_name $repository_workflow

        cd - &>/dev/null

        echo "Package $package_name updated successfuly"
    done
}

function load_secret() {
    if [ -f "$secret_path" ]; then
        secret=$(cat $secret_path)

        if [ ! -z "$secret" ]; then
            echo "$secret"
            return
        fi
    fi

    secret=$(text_prompt "Enter your github access token: ")
    echo "$secret" >$secret_path
    echo $secret
}

function main() {
    github_token=$(load_secret)

    local definitions=$(jq -c '.' "$definitions_path")

    for package_definition in $(jq -c '.[]' <<<"$definitions"); do
        make_release $package_definition
    done

    review
    apply_changes=$(boolean_prompt "Do you want to continue and apply these changes? [yes/no] ")

    if [ "$apply_changes" -eq 1 ]; then
        update_packages
    else
        echo "You did not continue, so the changes were not applied."
        exit 0
    fi
}

main
