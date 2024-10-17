#!/bin/bash

GITHUB_USER="singnet"

function get_server_details() {
    read -p "Enter the Server IP: " SERVER_IP
    read -p "Enter the Username: " USERNAME
}

function select_auth_method() {
    echo "Choose the authentication method:"
    echo "1) Password"
    echo "2) Private Key"
    read -p "Enter your choice [1-2]: " AUTH_METHOD
}

function ask_credentials() {
    if [ "$AUTH_METHOD" == "1" ]; then
        read -s -p "Enter your password: " PASSWORD
        echo
    else
        read -e -p "Enter the path to your private key: " PRIVATE_KEY
    fi
}

function fetch_github_repositories() {
    if ! command -v gh &> /dev/null; then
        echo "'gh' CLI is not installed. Please install it from 'https://cli.github.com'."
        return 1
    fi

    REPOS=$(gh repo list "$GITHUB_USER" --limit 100 --json name,sshUrl -q '.[] | "\(.name)"')

    if [ -z "$REPOS" ]; then
        echo "No repositories found for user/organization: $GITHUB_USER."
        return 1
    else
        echo "$REPOS"
    fi
}

function select_project() {
    PROJECTS=$(fetch_github_repositories)

    if [ $? -ne 0 ]; then
        echo "Unable to fetch repositories. Exiting..."
        exit 1
    fi

    IFS=$'\n' read -r -d '' -a PROJECT_ARRAY <<< "$PROJECTS"

    echo "Select the project:"
    select PROJECT in "${PROJECT_ARRAY[@]}"; do
        if [ -n "$PROJECT" ]; then
            break
        else
            echo "Invalid choice. Please select a project."
        fi
    done
}

function execute_commands_on_server() {
    local commands="$1"
    local ssh_command
    ssh_command=$( [[ "$AUTH_METHOD" == "1" ]] && echo "sshpass -p '$PASSWORD'" || echo "ssh -i '$PRIVATE_KEY'" )
    ssh_command+=" -o StrictHostKeyChecking=no $USERNAME@$SERVER_IP"

    eval "$ssh_command '$commands'"
}

function install_runner_requirements_on_server() {
    remote_commands="
        sudo mkdir -p $RUNNER_FOLDER_PATH && cd $RUNNER_FOLDER_PATH;
        sudo chmod -R 775 $RUNNER_FOLDER_PATH;
        sudo chown -R root:github-runner $RUNNER_FOLDER_PATH;
        curl -o actions-runner-linux-x64-2.320.0.tar.gz -L https://github.com/actions/runner/releases/download/v2.320.0/actions-runner-linux-x64-2.320.0.tar.gz;
        tar --strip-components=1 -xzf ./actions-runner-linux-x64-2.320.0.tar.gz;
    "

    execute_commands_on_server "$remote_commands"
}

function get_runner_token() {
    if ! command -v gh &> /dev/null; then
        echo "'gh' CLI is not installed. Please install it with 'https://cli.github.com'."
        exit 1
    fi

    if ! gh auth status &> /dev/null; then
        echo "Authenticating with GitHub..."
        gh auth login
    fi

    REPO_URL="https://github.com/$GITHUB_USER/$PROJECT"
    TOKEN=$(gh api -X POST "repos/$GITHUB_USER/$PROJECT/actions/runners/registration-token" | jq -r .token)

    if [ -z "$TOKEN" ]; then
        echo "Failed to retrieve the GitHub Actions runner token."
        exit 1
    fi
}

function create_github_runner_service() {
    remote_commands="sudo tee /etc/systemd/system/${RUNNER_FOLDER_NAME}.service > /dev/null <<EOF
[Unit]
Description=GitHub Runner Service for ${PROJECT}
After=network.target

[Service]
User=ubuntu
WorkingDirectory=${RUNNER_FOLDER_PATH}
ExecStart=${RUNNER_FOLDER_PATH}/run.sh
Restart=always

[Install]
WantedBy=multi-user.target
EOF
        sudo systemctl daemon-reload;
        sudo systemctl enable ${RUNNER_FOLDER_NAME};
        sudo systemctl start ${RUNNER_FOLDER_NAME};
    "

    execute_commands_on_server "$remote_commands"
}

function configure_runner_on_server() {
    remote_commands="
        cd $RUNNER_FOLDER_PATH;
        ./config.sh --url $REPO_URL --token $TOKEN;
    "

    execute_commands_on_server "$remote_commands"
}

function main() {
    get_server_details
    select_auth_method
    ask_credentials
    select_project

    RUNNER_FOLDER_NAME="${PROJECT}-runner"
    RUNNER_FOLDER_PATH="/opt/$RUNNER_FOLDER_NAME"

    echo "Entered data:"
    echo "Server IP: $SERVER_IP"
    echo "Username: $USERNAME"
    echo "Authentication Method: $( [[ "$AUTH_METHOD" == "1" ]] && echo "Password" || echo "Private Key (Path: $PRIVATE_KEY)" )"
    echo "Selected Project: $PROJECT"

    echo "Setting up runner on the server..."
    install_runner_requirements_on_server

    echo "Retrieving GitHub Actions runner token..."
    get_runner_token

    configure_runner_on_server

    create_github_runner_service
    echo "Runner setup complete!"
}

main "$@"