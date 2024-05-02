#!/bin/bash

set -e

# PATHS
workdir=$(pwd)
servers_file="$HOME/.das/servers.json"
github_token_path="$workdir/scripts/deployment/gh_token"

source "$workdir/scripts/deployment/utils.sh"

# GLOBAL VARIABLES
servers_definition=$(jq -c '.' "$servers_file")
github_token=$(load_or_request_github_token "$github_token_path")

function remove_server() {
    if [[ -f "$servers_file" ]]; then
        local aliases=($(jq -r '.servers[].alias' "$servers_file"))

        if [[ -z "$aliases" ]]; then
            print "\n:red:No server registed to be removed:/red:"
            exit 1
        fi

        local header=$(print_header "Remove Server")

        choose_menu "${header}Select the server you want to remove:" selected_option "${aliases[@]}"

        local existing_data=$(cat "$servers_file")

        if [[ $(echo "$existing_data" | jq '.servers') != "null" ]]; then
            existing_data=$(echo "$existing_data" | jq 'del(.servers[] | select(.alias == "'"$selected_option"'"))')

            echo "$existing_data" >"$servers_file"
            print "\n:green:Server removed successfully.:green:"
        else
            print "\n:red:There are no registered servers to remove.:/red:"
        fi
    else
        print "\n:red:The JSON file does not exist. No server was removed.:/red:"
    fi
}

function register_server() {
    clear
    print "$(print_header "Register Server")"

    local alias=$(text_prompt "Alias: ")
    local ip=$(text_prompt "IP: ")
    local username=$(text_prompt "Username: ")
    local pem_or_password=""
    local is_pem=false

    if boolean_prompt "Are you going to use a pem key to connect to server? [yes/no]: "; then
        while
            is_pem=true
            pem_or_password=$(text_prompt "Enter the path for the pem key file: ")

            if verify_file_exists "$pem_key"; then
                break
            fi
        do true; done
    else
        pem_or_password=$(password_prompt "Enter your password: ")
    fi

    print "\nTrying to establish a connection with server :green:${server_alias}:/green: (:green:${ip}:/green:)\n"
    if ! ping_ssh_server "$ip" "$username" "$is_pem" "$pem_or_password" &>/dev/null; then
        print ":red:A connection could not be successfully established!:/red:"
        exit 1
    fi

    append_server_data_to_json "$alias" "$ip" "$username" "$is_pem" "$pem_or_password" "$servers_file"

    print "\n\n:green:Server '$alias' succefully added.:/green:"
}

function append_server_data_to_json() {
    local alias="$1"
    local ip="$2"
    local username="$3"
    local is_pem="$4"
    local pem_or_password="$5"
    local servers_file="$6"

    local data='{"alias": "'"$alias"'", "ip": "'"$ip"'", "username": "'"$username"'", "is_pem": '"$is_pem"', "password": "'"$pem_or_password"'"}'

    if [[ -f "$servers_file" ]]; then
        local existing_data=$(cat "$servers_file")

        if [[ $(echo "$existing_data" | jq '.servers') != "null" ]]; then
            existing_data=$(echo "$existing_data" | jq '.servers += ['"$data"']')
        else
            existing_data=$(echo "$existing_data" | jq '. + {"servers": ['"$data"']}')
        fi

        echo "$existing_data" >"$servers_file"
    else
        local new_data='{"servers": ['"$data"']}'
        mkdir -p $(dirname "$servers_file")
        echo "$new_data" >"$servers_file"
    fi
}

function extract_server_details() {
    local servers="$1"
    local alias="$2"

    jq -r --arg alias "$alias" '.servers[] | select(.alias == $alias) | [.ip, .username, .is_pem, .password] | @tsv' <<<"$servers" | tr '\t' '|'
}

function get_server_das_cli_version() {
    local ip="$1"
    local username="$2"
    local using_private_key="$3"
    local pkey_or_password="$4"
    local command="das-cli --version"

    execute_ssh_commands "$ip" "$username" "$using_private_key" "$pkey_or_password" "$command"

    return $?
}

function get_server_function_version() {
    local ip="$1"
    local username="$2"
    local using_private_key="$3"
    local pkey_or_password="$4"
    local command="das-cli faas version"

    execute_ssh_commands "$ip" "$username" "$using_private_key" "$pkey_or_password" "$command"

    return $?
}

function get_function_versions() {
    local page=1
    local versions=()

    while true; do
        local api_url="https://api.github.com/repos/singnet/das-serverless-functions/tags?page=${page}"

        local response=$(curl -s -X GET \
            -H "Authorization: token ${github_token}" \
            -H "Accept: application/vnd.github.v3+json" \
            "${api_url}")

        if [[ $(jq '. | length' <<<"$response") -eq 0 ]]; then
            break
        fi

        local tags=$(jq -r '.[].name | select(test("^[0-9]+\\.[0-9]+\\.[0-9]+$"))' <<<"${response}")

        ((page++))

        if [[ -n "$tags" ]]; then
            versions+=("$tags")
        fi
    done

    echo "${versions[@]}"
}

function set_server_function_version() {
    local ip="$1"
    local username="$2"
    local private_key_connection="$3"
    local private_key_path="$4"
    local function_version="$5"
    local command="das-cli faas update-version --version ${function_version}; das-cli db start; das-cli faas restart"

    execute_ssh_commands "$ip" "$username" "$private_key_connection" "$private_key_path" "$command"

    return $?
}

function start_deployment() {
    local header=$(print_header "START DEPLOYMENT")
    local aliases=($(jq -r '.servers[].alias' "$servers_file"))

    if [[ -z "$aliases" ]]; then
        print "\n:red:No server registed to be started, please register a new server:/red:"
        exit 1
    fi

    choose_menu "${header}Which server would you like to deploy to?" server_alias "${aliases[@]}"

    IFS='|' read -r ip username is_pem password <<<"$(extract_server_details "$servers_definition" "$server_alias")"

    print "\nTrying to establish a connection with server :green:${server_alias}:/green: (:green:${ip}:/green:)\n"
    if ! ping_ssh_server "$ip" "$username" "$is_pem" "$password" &>/dev/null; then
        print ":red:A connection could not be successfully established!:/red:"
        exit 1
    fi

    local server_das_cli_version=$(get_server_das_cli_version "$ip" "$username" "$is_pem" "$password")
    local server_function_version=$(get_server_function_version "$ip" "$username" "$is_pem" "$password")
    local function_versions=($(get_function_versions))

    if [[ -z "${function_versions[@]}" || ${#function_versions[@]} -le 0 ]]; then
        print ":red:There are no available functions to be deployed:/red:"
        exit 1
    fi

    choose_menu "${header}DAS CLI: :green:${server_das_cli_version}:/green:\nFUNCTION: :green:${server_function_version}:/green:\n\nSelect a version you want to deploy to the server :green:${server_alias}:/green: (:green:${ip}:/green:)" function_version "${function_versions[@]}"

    print "\nDeploying the version ${function_version} to server ${server_alias} (${ip})..."

    if set_server_function_version "$ip" "$username" "$is_pem" "$password" "$function_version"; then
        print "\n:green:Deployment successful: Version $function_version deployed to server $ip ($username).:green:\n"
    else
        print "\n:red:Deployment failed: Version $function_version could not be deployed to server $ip ($username).:red:\n"
    fi
}

function main() {
    local options=(
        "Register Server"
        "Remove Server"
        "Start Deployment"
        "Quit"
    )

    local header=$(print_header "OpeenFaaS Deployment Tool")

    choose_menu "${header}Please choose an option:" selected_option "${options[@]}"

    case $selected_option in
    "Register Server")
        register_server
        ;;
    "Remove Server")
        remove_server
        ;;
    "Start Deployment")
        start_deployment
        ;;
    "Quit")
        print ":yellow:\nQuitting...:yellow:"
        exit 0
        ;;
    esac

}

main
