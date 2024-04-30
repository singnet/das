#!/bin/bash

set -e

# PATHS
workdir=$(pwd)
servers_file="$HOME/.das/servers.json"

source "$workdir/scripts/deployment/utils.sh"

# START DEPLOYMENT
# lista de servers (alias)
# tentar se conectar ao server
# identificar qual a versão de function que o server está executando
# identificar a versão do das-cli disponivel
# buscar lista de versões disponiveis
# exibir lista de versões disponiveis para o usuário selecionar, exibindo a versão atual que está executando no servidor
# executar o das-cli no server com o comando das-cli faas start --version

# REGISTER SERVER
# alias
# 127.0.0.1
# ROOT
# PEM OR PASSWORD

function remove_server() {
    if [[ -f "$servers_file" ]]; then
        local aliases=($(jq -r '.servers[].alias' "$servers_file"))

        if [[ -z "$aliases" ]]; then
            print ":red:No server registed to be removed:/red:"
            exit 1
        fi

        print_header "REMOVE SERVER"
        choose_menu "Select the server you want to remove:" selected_option "${aliases[@]}"

        local existing_data=$(cat "$servers_file")

        if [[ $(echo "$existing_data" | jq '.servers') != "null" ]]; then
            existing_data=$(echo "$existing_data" | jq 'del(.servers[] | select(.alias == "'"$selected_option"'"))')

            echo "$existing_data" >"$servers_file"
            print ":green:Server removed successfully.:green:"
        else
            print ":red:There are no registered servers to remove.:/red:"
        fi
    else
        print ":red:The JSON file does not exist. No server was removed.:/red:"
    fi
}

function register_server() {
    print_header "REGISTER SERVER"

    local alias=$(text_prompt "Alias: ")
    local ip=$(text_prompt "IP: ")
    local username=$(text_prompt "Username: ")
    local pem_or_password=""
    local is_pem=false

    if boolean_prompt "Are you going to use a pem key to connect to the server? [yes/no]: "; then
        while
            is_pem=true
            pem_or_password=$(text_prompt "Enter the path for the pem key file: ")

            if verify_file_exists "$pem_key"; then
                break
            fi
        do true; done
    else
        password=$(password_prompt "Enter your password: ")
    fi

    ping_ssh_server "$ip" "$username" "$is_pem" "$pem_or_password"
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

function start_deployment() {
    local aliases=($(jq -r '.servers[].alias' "$servers_file"))

    if [[ -z "$aliases" ]]; then
        print ":red:No server registed to be removed:/red:"
        exit 1
    fi

    print_header "START DEPLOYMENT"

    choose_menu "Please choose a server for you to deploy:" selected_option "${aliases[@]}"

    echo "$selected_option"
}

function main() {
    local options=(
        "Register Server"
        "Remove Server"
        "Start Deployment"
        "Exit"
    )

    choose_menu "Please choose an option:" selected_option "${options[@]}"

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
    "Exit")
        echo "Exiting..."
        exit 0
        ;;
    esac
}

main
