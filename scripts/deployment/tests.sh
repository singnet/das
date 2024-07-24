#!/bin/bash

set -e

# PATHS
workdir=$(pwd)
test_definitions_path="$workdir/scripts/deployment/test_definitions.json"

# GLOBAL VARIABLES
required_commands=(git jq curl)
test_definitions=$(jq -c '.' "$test_definitions_path")

source "$workdir/scripts/deployment/utils.sh"
source "$workdir/scripts/deployment/workflow.sh"

function extract_test_package_details() {
    local package="$1"
    jq -r '[.repository, .workflow, .ref] | @tsv' <<<"$package" | tr '\t' '|'
}

function run_integration_tests() {
    print "Initiating integration testing..."

    for test_definition in $(jq -c '.[]' <<<"$test_definitions"); do
        IFS='|' read -r repository workflow ref <<<"$(extract_test_package_details "$test_definition")"

        validate_repository_url "$repository"

        local repository_owner="${BASH_REMATCH[1]}"
        local repository_name="${BASH_REMATCH[2]}"
        local package_name="$repository_name"
        local workflow_inputs="{}"
        local repository_workflow="$workflow"
        local repository_ref="$ref"

        trigger_package_workflow "$package_name" "$workflow_inputs" "$repository_owner" "$repository_name" "$repository_workflow" "$repository_ref"
        print ":green:Integration tests completed successfully:/green:"
    done
}

function main() {
    requirements "${required_commands[@]}"

    run_integration_tests
}

main