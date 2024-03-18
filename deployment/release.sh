#!/bin/bash

set -e

definitions_path="$(pwd)/deployment/definitions.json"

function boolean_prompt() {
    prompt="$1"

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
    prompt="$1"

    read -p "$prompt" answer

    echo $answer
}

function get_latest_version() {
    echo "v1.0.0"
}

function update_package_dependencies() {
    package_dependencies=$1
    package_name=$(jq -r '.name' <<< "$package_definition")

    if [ "$package_dependencies" = "null" ] || [ "${#package_dependencies[@]}" -eq 0 ]; then
        echo "Skipping dependency update for package $package_name as it has no dependent packages."
        return
    fi

    echo "Updated" 

}

function fmt_version() {
    local version="$1"
    local pattern="^[0-9]+\.[0-9]+\.[0-9]+$"

    if [ "$version" == "$pattern" ]; then
        echo 1
        return
    fi

    echo 0
    return
}

function make_release() {
    package_definition="$1"

    package_name=$(jq -r '.name' <<< "$package_definition")
    package_dependencies=$(jq -r '.dependencies' <<< "$package_definition")

    create_release=$(boolean_prompt "Do you want to create a release for ${package_name}: [yes/no] ")

    if [ "$create_release" -eq 1 ]; then
        update_package_dependencies $package_dependencies
    else
        echo "Skipping release for $package_name"
    fi

    lastest_package_version=$(get_latest_version)
    new_package_version=$(text_prompt "The $package_name current version is $lastest_package_version. What is the next version you want to release? (v1.0.0) ")

    
}

function main() {
    definitions=$(jq -c '.' "$definitions_path")

    for package_definition in $(jq -c '.[]' <<< "$definitions"); do
        make_release $package_definition
    done
}

main