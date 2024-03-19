#!/bin/bash

set -e


definitions_path="$(pwd)/deployment/definitions.json"

function boolean_prompt() {
    local prompt="$1"

    while true; do
        read -p "$prompt" answer
        answer=$(echo "$answer" | tr '[:upper:]' '[:lower:]')

        case "${answer}" in
            "y" | "yes")
                echo 1
                return;;
            "n" | "no")
                echo 0
                return;;
            *)
                continue;;
        esac
    done
}

function text_prompt() {
    local prompt="$1"

    read -p "$prompt" answer

    echo $answer
}

function get_repository_latest_version() {
    local package_repository_folder="$1"

    cd $package_repository_folder
    local latest_version=$(git tag | sort -V | tail -n 1)

    cd - &>/dev/null
    if [ -z $latest_version ]; then
        echo "0.0.0"
    fi

    echo $latest_version
}

function update_package_dependencies() {
    local package_dependencies=$1
    local package_name=$(jq -r '.name' <<< "$package_definition")

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
    local lastest_package_version=$(get_repository_latest_version $package_repository_folder)

    while true; do
        new_package_version=$(text_prompt "The $package_name current version is $lastest_package_version. What is the next version you want to release? (0.0.0) ")
        
        is_valid_version=$(fmt_version $new_package_version)

        if [ "$is_valid_version" -eq 0 ]; then
            echo "The version '$new_package_version' you provided is invalid"
            continue
        fi

        # run hook here

        # set up git
        git add .
        git commit -m ""
        return
    done
}

function clone_repository() {
    local package_repository=$1
    local tmp_folder=$(mktemp -d)

    git clone $package_repository $tmp_folder

    echo $tmp_folder
}

function make_release() {
    local package_definition="$1"

    local package_name=$(jq -r '.name' <<< "$package_definition")
    local package_repository=$(jq -r '.repository' <<< "$package_definition")
    local package_dependencies=$(jq -r '.dependencies' <<< "$package_definition")

    local create_release=$(boolean_prompt "Do you want to create a release for ${package_name}: [yes/no] ")

    if [ "$create_release" -eq 1 ]; then
        update_package_dependencies $package_dependencies
    else
        echo "Skipping release for $package_name"
    fi

    local package_repository_folder=$(clone_repository $package_repository)
    make_tag $package_repository_folder
}

function main() {
    local definitions=$(jq -c '.' "$definitions_path")

    for package_definition in $(jq -c '.[]' <<< "$definitions"); do
        make_release $package_definition
    done
}

main