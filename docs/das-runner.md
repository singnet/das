### How to Configure a GitHub Actions Runner on Your Server

This guide explains how to manually configure a self-hosted GitHub Actions runner on your server. A runner allows you to execute CI/CD jobs from GitHub Actions on your infrastructure instead of using GitHub-hosted runners.

#### Prerequisites

- Access to a server (SSH login).
- GitHub CLI (`gh`) installed.
- User with `admin` access to the repository where you want to configure the runner.
- Sudo or root access on the server.
- Basic familiarity with Linux commands and system administration.

---

### Steps to Configure a GitHub Actions Runner

#### 1. **Login to the GitHub CLI**

First, ensure that you have `gh` installed on your local machine.

```bash
gh auth login
```

This command will authenticate your GitHub account. Follow the prompts to complete the authentication process.

#### 2. **Generate a GitHub Actions Runner Token**

Next, you will need a registration token to link the runner to a repository.

```bash
gh api -X POST "repos/<user_or_org>/<repo>/actions/runners/registration-token" | jq -r .token
```

Replace `<user_or_org>` and `<repo>` with the name of the user or organization and the repository where the runner will be linked. Save the token somewhere safe for later use.

#### 3. **Connect to Your Server**

SSH into the server where you want to install the GitHub Actions runner.

```bash
ssh <username>@<server_ip>
```

Replace `<username>` and `<server_ip>` with your server's credentials.

#### 4. **Download and Install the GitHub Actions Runner**

On the server, create a directory where the runner will be installed. Typically, this will be placed under `/opt`.

```bash
sudo mkdir -p /opt/actions-runner && cd /opt/actions-runner
```

Download the runner package from GitHub’s release page:

```bash
curl -o actions-runner-linux-x64-2.320.0.tar.gz -L https://github.com/actions/runner/releases/download/v2.320.0/actions-runner-linux-x64-2.320.0.tar.gz
```

Verify the download by checking its SHA-256 checksum:

```bash
echo '93ac1b7ce743ee85b5d386f5c1787385ef07b3d7c728ff66ce0d3813d5f46900  actions-runner-linux-x64-2.320.0.tar.gz' | shasum -a 256 -c
```

Extract the package:

```bash
tar xzf ./actions-runner-linux-x64-2.320.0.tar.gz --strip-components=1
```

#### 5. **Configure the Runner**

Run the configuration script to link the runner to your GitHub repository using the token you retrieved earlier:

```bash
./config.sh --url https://github.com/<user_or_org>/<repo> --token <runner_token>
```

Replace `<user_or_org>`, `<repo>`, and `<runner_token>` with the appropriate values.

#### 6. **Set Up the Runner as a Systemd Service**

To ensure the runner starts automatically and runs in the background, create a systemd service for it. Here’s how to do it:

```bash
sudo tee /etc/systemd/system/github-runner.service > /dev/null <<EOF
[Unit]
Description=GitHub Runner Service
After=network.target

[Service]
User=ubuntu
WorkingDirectory=/opt/actions-runner
ExecStart=/opt/actions-runner/run.sh
Restart=always

[Install]
WantedBy=multi-user.target
EOF
```

**Note:** If you installed the runner in a custom directory, update the `WorkingDirectory` and `ExecStart` paths accordingly.

#### 7. **Start and Enable the Service**

Reload the systemd manager to acknowledge the new service and start the runner:

```bash
sudo systemctl daemon-reload
sudo systemctl enable github-runner
sudo systemctl start github-runner
```

You can check the runner's status with:

```bash
sudo systemctl status github-runner
```

#### 8. **Verify the Runner on GitHub**

Go to your GitHub repository and navigate to **Settings > Actions > Runners**. You should see the new runner listed as "online."

---

### Optional: Run the Runner Without a Service

If you don’t want to create a systemd service, you can simply run the runner in the foreground by executing:

```bash
./run.sh
```

This approach requires the terminal to remain open for the runner to keep working.

---

### Troubleshooting

- **Runner not appearing in GitHub**: Ensure that the server has access to the internet and that there are no firewall restrictions blocking GitHub Actions from communicating with your runner.
- **Runner service failed to start**: Check the logs using `journalctl -u github-runner` for more information.
- **Token expired**: Registration tokens are short-lived. If you get an error about the token being expired, generate a new one by repeating Step 2.

---

### Conclusion

You’ve successfully set up a self-hosted GitHub Actions runner on your server. It will now be available to execute jobs in the specified repository. If you want to scale up, you can repeat the process to add more runners, or manage multiple repositories with different configurations!