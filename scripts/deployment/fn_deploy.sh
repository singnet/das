#!/bin/bash

set -e

# PATHS
workdir=$(pwd)
das_dir="$HOME/.das"

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

function display_server_aliases() {
    local json_filename="$1"
    local aliases=$(jq -r '.servers[].alias' "$json_filename")
    echo "Servidores registrados:"
    echo "$aliases"
}

function remove_server() {
    echo -e "\nRemove Server\n"

    local das_dir="$HOME/.das"
    local json_filename="$das_dir/data.json"

    if [[ -f "$json_filename" ]]; then
        display_server_aliases "$json_filename"

        local alias_to_remove=$(text_prompt "Alias do servidor a ser removido: ")
        local existing_data=$(cat "$json_filename")

        if [[ $(echo "$existing_data" | jq '.servers') != "null" ]]; then
            existing_data=$(echo "$existing_data" | jq 'del(.servers[] | select(.alias == "'"$alias_to_remove"'"))')

            echo "$existing_data" >"$json_filename"
            echo "Servidor removido com sucesso."
        else
            echo "Não há servidores registrados para remover."
        fi
    else
        echo "O arquivo JSON não existe. Nenhum servidor foi removido."
    fi
}

function register_server() {
    print "\nRegister Server\n"

    local alias=$(text_prompt "Alias: ")
    local ip=$(text_prompt "IP: ")
    local username=$(text_prompt "Username: ")
    local pem_or_password=$(text_prompt "Pem key or Password: ")
    local is_pem=""
    local json_filename="$das_dir/data.json"

    if is_pem_file "$pem_or_password" &>/dev/null; then
        is_pem=true
    else
        is_pem=false
        pem_or_password=$(encrypt_password "$pem_or_password" "mykey")
    fi

    append_server_data_to_json "$alias" "$ip" "$username" "$is_pem" "$pem_or_password" "$json_filename"
}

function append_server_data_to_json() {
    local alias="$1"
    local ip="$2"
    local username="$3"
    local is_pem="$4"
    local pem_or_password="$5"
    local json_filename="$6"

    local data='{"alias": "'"$alias"'", "ip": "'"$ip"'", "username": "'"$username"'", "is_pem": '"$is_pem"', "password": "'"$pem_or_password"'"}'

    if [[ -f "$json_filename" ]]; then
        local existing_data=$(cat "$json_filename")

        if [[ $(echo "$existing_data" | jq '.servers') != "null" ]]; then
            existing_data=$(echo "$existing_data" | jq '.servers += ['"$data"']')
        else
            existing_data=$(echo "$existing_data" | jq '. + {"servers": ['"$data"']}')
        fi

        echo "$existing_data" >"$json_filename"
    else
        local new_data='{"servers": ['"$data"']}'
        mkdir -p "$das_dir"
        echo "$new_data" >"$json_filename"
    fi

    echo "Dados salvos com sucesso em: $json_filename"
}

function start_deployment() {
    echo ""
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
        echo "Saindo..."
        exit 0
        ;;
    esac
}

main
