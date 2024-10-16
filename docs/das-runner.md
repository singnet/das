# GitHub Self-Hosted Runner Setup Guide

This guide walks through the steps required to configure a self-hosted GitHub Actions runner for projects in the `singnet` organization using the `github-runner` script.

## Script Overview

The script automates the process of:
1. Connecting to a remote server (using either password or SSH key authentication).
2. Fetching the list of GitHub repositories.
3. Selecting a repository to configure the runner for.
4. Installing the GitHub Actions runner on the server.
5. Configuring the runner and creating a `systemd` service to manage it.

## Pre-requisites

Before running the script, ensure the following:
1. The `gh` CLI is installed and authenticated with GitHub.
   - You can install the `gh` CLI by following [this link](https://cli.github.com/).
2. You have SSH access to the server where the runner will be installed.
3. `jq` is installed on the local machine to parse JSON responses.

### Installing `gh` and `jq`

```bash
# Install GitHub CLI
sudo apt install gh

# Install jq
sudo apt install jq
```

## Usage Instructions

### 1. Running the Script

Start the script with the following command:

```bash
make github-runner
```

### 2. Enter Server Details

The script will prompt you for the following details:
- **Server IP:** The IP address of the server where the GitHub runner will be installed.
- **Username:** The username for SSH access to the server.

### 3. Authentication Method

Select the method to authenticate on the server:
- **1) Password:** Use a password for SSH.
- **2) Private Key:** Use a private key for SSH.

Depending on your choice, you will either be prompted for your password or the path to your SSH private key.

### 4. Fetching Repositories

The script will use the `gh` CLI to list all repositories in the `singnet` organization. It will display the list, and you can select the repository for which the runner will be configured.

### 5. Installing Runner on the Server

After selecting the project, the script will:
- Create a folder on the server to hold the GitHub Actions runner files.
- Download and extract the GitHub Actions runner software.
- Set the correct permissions for the runner files.

### 6. Retrieving Runner Token

The script will generate a registration token required to link the runner to the selected GitHub repository. This step is handled via the GitHub CLI.

### 7. Configuring the Runner on the Server

The runner is configured on the remote server by executing the necessary setup commands, such as linking it to the chosen repository and setting up the `systemd` service for automatic management.

### 8. Creating a Systemd Service

The script creates a `systemd` service file on the server to manage the GitHub Actions runner as a service. This allows the runner to automatically start on boot and restart if it crashes.

### 9. Commands for Managing the Runner

Once the runner is set up, you can use the following commands on the server to manage it:

- **Start the runner:**
  ```bash
  sudo systemctl start <runner-service-name>
  ```

- **Check the runner status:**
  ```bash
  sudo systemctl status <runner-service-name>
  ```

- **Stop the runner:**
  ```bash
  sudo systemctl stop <runner-service-name>
  ```

Replace `<runner-service-name>` with the actual name of the service, which is derived from the project name (e.g., `myproject-runner`).
