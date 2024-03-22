#!/bin/bash

set -e

workdir=$(pwd)
hooks_path="$workdir/deployment/hooks"
definitions_path="$workdir/deployment/definitions.json"
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
    local latest_version=$(git tag | sort -V | tail -n 1)

    if [ -z $latest_version ]; then
        echo "0.0.0"
    fi

    echo $latest_version
}

function update_package_dependencies() {
    local package_dependencies=$1
    local package_name=$(jq -r '.name' <<<"$package_definition")

    if [ "$package_dependencies" = "null" ] || [ "${#package_dependencies[@]}" -eq 0 ]; then
        echo "Skipping dependency update for package $package_name as it has no dependent packages."
        return
    fi

    echo "Updated"

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

function make_tag() {
    local package_repository_folder="$1"
    local package_hook_before_release="$hooks_path/$2"
    local new_package_version=""

    cd $package_repository_folder

    local latest_package_version=$(get_repository_latest_version)

    while true; do
        new_package_version=$(text_prompt "The $package_name current version is $latest_package_version. What is the next version you want to release? (0.0.0) ")

        is_valid_version=$(fmt_version $new_package_version)

        if [ "$is_valid_version" -eq 0 ]; then
            echo "The version '$new_package_version' you provided is invalid"
            continue
        fi

        source $package_hook_before_release &>/dev/null

        local versions=("$latest_package_version" "$new_package_version")

        echo "${versions[@]}"

        return
    done

    cd - &>/dev/null
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

function get_packages_pending_update() {
    local index=$1
    local key=$2
    echo "$packages_pending_update" | jq -r ".[$index].$key"
}

function make_release() {
    local package_definition="$1"

    local package_name=$(jq -r '.name' <<<"$package_definition")
    local package_repository=$(jq -r '.repository' <<<"$package_definition")
    local package_dependencies=$(jq -r '.dependencies' <<<"$package_definition")
    local package_hook_before_release=$(jq -r '.hooks.beforeRelease' <<<"$package_definition")

    local create_release=$(boolean_prompt "Do you want to create a release for ${package_name}: [yes/no] ")

    if [ "$create_release" -eq 1 ]; then
        update_package_dependencies $package_dependencies
    else
        echo "Skipping release for $package_name"
    fi

    local package_repository_folder=$(clone_repository $package_repository)
    local versions=($(make_tag $package_repository_folder $package_hook_before_release))

    new_package="{\"package_name\": \"${package_name}\", \"current_version\": \"${versions[0]}\", \"new_version\": \"${versions[1]}\", \"repository_path\": \"${package_repository_folder}\"}"

    push_to_packages_pending_update "$new_package"
}

function commit_changes() {
    commit_msg="$1"

    setup_git

    git commit -m "$commit_msg"
}

function setup_git() {
    git_user=$(git config user.name)
    git_email=$(git config user.email)

    while [ -z "$git_user" ] || [ -z "$git_email" ]; do
        git_user=$(text_prompt "Your Git username is not configured. Please provide your Git name: ")
        git_email=$(text_prompt "Your Git email is not configured. Please provide your Git email: ")

        git config user.name "$git_user"
        git config user.email "$git_email"
    done
}

function review() {
    for package in $(jq -c '.[]' <<<"$packages_pending_update"); do
        local package_name=$(jq -r '.package_name' <<<"$package")
        local package_current_version=$(jq -r '.current_version' <<<"$package")
        local package_new_version=$(jq -r '.new_version' <<<"$package")

        echo "$package_name from version $package_current_version to version $package_new_version"
    done
}

function main() {
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
