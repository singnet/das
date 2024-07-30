# Deployment and Release Documentation

## Introduction

This documentation describes the deployment and release process for the **DAS** project. It covers the scripts located in `scripts/deployment` and the complete flow for generating new releases, performing integration tests, and deploying functions to servers. The provided information includes details about the utilized scripts, necessary configurations, and steps to execute each process efficiently.

## Deployment and Release Scripts

Within the `scripts/deployment` directory, there are various scripts responsible for deploying and generating releases. At the root of the project, you can run the `make release` command to initiate the process of creating a new release. This interactive script guides the user through a series of questions for each repository defined in the `definitions.json` file.

### Repository Definitions

The `definitions.json` file contains a list of repositories that will be cloned to the user's machine. It is important to ensure that the environment is configured with SSH keys that have permission to access all these repositories. Each entry in the JSON includes the following fields:

- **name**: A unique name for the repository, used as an identifier in the script.
- **repository**: The SSH URL of the repository.
- **workflow**: The name of the workflow file to be used (e.g., `build.yml`).
- **ref**: The reference for the branch, tag, or commit hash.
- **hooks**: Custom scripts that can be executed at different stages of the process.
- **dependencies**: A list of repository names that the current repository depends on.

The script prompts the user for the desired version for each repository, and the new version is applied after defining all versions. The order of definitions in the `definitions.json` file is crucial, especially for dependent repositories.

### Available Hooks

Hooks are scripts that can be executed at specific points during the release process. Some available hooks include:

- **beforeRelease**: Executed before generating the repository's version. For example, in `das-toolbox`, this hook updates the configuration file with the new version.
- **dependencyCheck**: This hook is executed for each dependency listed in the repository's `dependencies` field. For example, in the case of `hyperon-das`, the `dependencyCheck` hook can update the version of `hyperon-das-atomdb`, a dependency of `hyperon-das`, ensuring that the latest version is used in the new release.

### Authentication Configuration

When the release script is executed for the first time, a personal GitHub token is requested. This token must have repository and workflow permissions. The token is stored in a text file without an extension, called `gh_token`, within `scripts/deployment`. To use a new token, simply delete the existing file, and the script will prompt for a new token on the next run. If an authorization error occurs, the token may have been revoked or expired, requiring a new one to be generated.

### Backup and Recovery

The release script has a backup system that saves the current state of the process in a file called `.output~` at the project's root. If the script is interrupted, it can be resumed from where it left off. When the script is restarted, it reads the data from the backup file and continues from that point. It's important to note that inputs are saved at checkpoints, so if the script stops during the execution of a workflow, all workflows will be executed again upon restarting.

## Integration Tests

After updating all versions, the release script will ask the user if they want to run the integration tests. If confirmed, the test script will be executed. To run the tests separately, use the `make integration-tests` command. The test definition file, `test_definitions.json`, located in `scripts/deployment`, contains:

- **repository**: The SSH URL of the repository.
- **workflow**: The name of the workflow file containing the tests.
- **ref**: The reference for the branch, tag, or commit hash.

The tests are executed in the defined workflows, ensuring the integrity of the new release.

## Function Deployment

To deploy functions, use the `make deploy` command. This interactive script allows for the registration of servers for deployment. When registering a server, provide an alias name, the server's IP address, the username, and the path to the private key, if necessary. The registered server information is saved in `~/.das/servers.json`. During deployment, an SSH connection is established with the server, and a list of available function versions is displayed. Select the desired version, and the deployment will be performed. The server must have the `das-cli` installed to execute the deployment.

For complete releases, it is advisable to deploy the functions to the development environment before running the integration tests.
