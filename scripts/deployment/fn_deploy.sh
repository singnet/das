#!/bin/bash

set -e

# PATHS
workdir=$(pwd)
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

function register_server() {
    echo ""
}

function start_deployment() {
    echo ""
}

function main() {
    local options=(
        "Register Server"
        "Start Deployment"
        "Exit"
    )

    choose_menu "Please choose an option:" selected_option "${options[@]}"

    case $selected_option in
    "Start Deployment")
        start_deployment
        ;;
    "Register Server")
        register_server
        ;;
    "Exit")
        echo "Saindo..."
        exit 0
        ;;
    esac
}

main
