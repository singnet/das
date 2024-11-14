#!/bin/bash

set -e

# PATHS
workdir=$(pwd)
github_token_path="$workdir/scripts/deployment/gh_token"

source "$workdir/scripts/deployment/utils.sh"

# GLOBAL VARIABLES
required_commands=(git jq curl)
github_token=$(load_or_request_github_token "$github_token_path")

function trigger_workflow() {
    local inputs="$1"
    local repository_owner="$2"
    local repository_name="$3"
    local repository_workflow="$4"
    local repository_ref="$5"
    local payload=""

    local api_url="https://api.github.com/repos/${repository_owner}/${repository_name}/actions/workflows/${repository_workflow}/dispatches"

    if [ "$inputs" = "{}" ] || [ -z "$inputs" ]; then
        payload="{\"ref\": \"$repository_ref\"}"
    else
        payload="{\"ref\": \"$repository_ref\", \"inputs\": $inputs}"
    fi

    local response_code=$(curl -s -X POST \
        -H "Authorization: token $github_token" \
        -H "Accept: application/vnd.github.v3+json" \
        "$api_url" \
        -d "$payload" \
        -o /dev/null -w "%{http_code}")

    if [[ "$response_code" -ne 200 && "$response_code" -ne 204 ]]; then
        print "Failed to trigger workflow: HTTP status $response_code"
        return 1
    else
        print "Workflow dispatch initiated successfully."
    fi
}

function monitor_workflow_status() {
    local repository_owner="$1"
    local repository_name="$2"
    local repository_workflow="$3"

    local max_retries=30
    local retry_count=0

    print "Monitoring workflow execution status..."

    while [[ $retry_count -lt $max_retries ]]; do
        sleep 10

        local status_response=$(curl -s -X GET \
            -H "Authorization: token $github_token" \
            -H "Accept: application/vnd.github.v3+json" \
            "https://api.github.com/repos/${repository_owner}/${repository_name}/actions/workflows/${repository_workflow}/runs?event=workflow_dispatch")

        local repository_workflow_path=".github/workflows/$repository_workflow"

        local status=$(echo "${status_response}" | jq -r '.workflow_runs[] | select(.path == "'"${repository_workflow_path}"'") | .conclusion' | head -n 1)

        if [[ -n "${status}" ]]; then
            case "${status}" in
            "success")
                print ":green:Workflow completed successfully.:/green:"
                return 0
                ;;
            "failure")
                print ":red:Workflow failed.:red:"
                return 1
                ;;
            "cancelled")
                print ":red:Workflow cancelled.:red:"
                return 1
                ;;
            *)
                print "Workflow is currently running for :green:${repository_name}:/green:. Please wait..."
                ;;
            esac
        else
            print "Waiting for the workflow to start..."
        fi

        ((retry_count++))
    done

    if [[ $retry_count -eq $max_retries ]]; then
        print ":red:Workflow did not complete in the expected time.:/red:"
        return 1
    fi
}

function trigger_package_workflow() {
    local package_name="$1"
    local workflow_inputs="$2"
    local repository_owner="$3"
    local repository_name="$4"
    local repository_workflow="$5"
    local repository_ref="$6"

    if trigger_workflow "$workflow_inputs" "$repository_owner" "$repository_name" "$repository_workflow" "$repository_ref"; then
        monitor_workflow "$package_name" "$repository_owner" "$repository_name" "$repository_workflow"
    else
        handle_workflow_trigger_failure "$package_name"
    fi
}

function monitor_workflow() {
    local package_name="$1"
    local repository_owner="$2"
    local repository_name="$3"
    local repository_workflow="$4"

    if ! monitor_workflow_status "$repository_owner" "$repository_name" "$repository_workflow"; then
        handle_workflow_failure "$package_name"
    fi
}

function handle_workflow_trigger_failure() {
    local package_name="$1"

    if boolean_prompt "The workflow could not be triggered. Do you want to continue? You can trigger it manually and then continue by typing 'yes', or exit by typing 'no'. Continue? [yes/no]: "; then
        print "Continuing after manual intervention..."
    else
        print ":yellow:Exiting as requested.:/yellow:"
        exit 0
    fi
}

function handle_workflow_failure() {
    local package_name="$1"

    if boolean_prompt "Something went wrong and the workflow failed... You need to monitor it manually at GitHub page. Once it finishes, you can type 'yes' to continue or 'no' to not continue and exit now. Continue? [yes/no]: "; then
        print ":yellow:Continuing after manual verification...:/yellow:"
    else
        print ":yellow:Exiting as requested.:/yellow:"
        exit 0
    fi
}