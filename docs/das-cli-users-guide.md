# DAS CLI - User's Guide

This guide provides a comprehensive, hands-on walkthrough for using the **DAS CLI (`das-cli`)** to set up, manage, and interact with the Distributed Atomspace (DAS) ecosystem. It is designed to take you from initial setup to running a complete DAS environment, focusing on practical, real-world command usage.

---

## Table of Contents
- [DAS CLI - User's Guide](#das-cli---users-guide)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Prerequisites](#prerequisites)
  - [Installation](#installation)
    - [1. For Linux Users (Debian-based systems like Ubuntu)](#1-for-linux-users-debian-based-systems-like-ubuntu)
      - [Debian Package (Recommended)](#debian-package-recommended)
      - [From Source](#from-source)
      - [Binary Release](#binary-release)
    - [2. For MacOS/Windows Users (Remote Execution)](#2-for-macoswindows-users-remote-execution)
  - [Quickstart Guides](#quickstart-guides)
    - [Step 1: Configure DAS CLI](#step-1-configure-das-cli)
    - [Step 2: Start the Core Databases](#step-2-start-the-core-databases)
    - [Step 3: Load a Knowledge Base](#step-3-load-a-knowledge-base)
    - [Step 4: Start Agents](#step-4-start-agents)
    - [Step 5: Check the Logs](#step-5-check-the-logs)
  - [Updating `das-cli` and Handling Configuration Changes](#updating-das-cli-and-handling-configuration-changes)
  - [Core Funcionality Overview](#core-funcionality-overview)
    - [Managing Agents](#managing-agents)
    - [Managing Databases](#managing-databases)
    - [Managing MeTTa Files](#managing-metta-files)
    - [Managing Configuration](#managing-configuration)
      - [Configurable Parameters](#configurable-parameters)
    - [Monitoring with Logs](#monitoring-with-logs)

---

## Introduction

**DAS CLI (`das-cli`)** is the primary command-line tool for deploying, configuring, and managing Distributed Atomspace (DAS). It simplifies the orchestration of essential components, including containerized databases (Redis, MongoDB), knowledge base management (MeTTa files), and AI agents.

This guide will walk you through setting up your environment, loading data, running services, and monitoring their status.

> [!NOTE]
>
> While the `das-cli` tool is cross-platform and runs on Linux, MacOS, and Windows, the core DAS components it manages (like AI agents) are designed to operate in a **Linux environment**.
>
> This means you can use `das-cli` in different ways depending on your operating system:
>
> -   **On a Linux machine:** You have the most flexibility. You can use `das-cli` to manage DAS components locally on the same machine or to connect to another Linux host and manage it remotely using the `--remote` flag.
> -   **On MacOS or Windows:** On these systems, `das-cli` functions exclusively as a remote control. You use it to send commands from your local machine to a designated Linux host where the DAS components are actually running. This is always done using the `--remote` flag.
>
> This guide provides instructions for all scenarios.

---

## Prerequisites

Before you begin, ensure you have the following installed and running:

- **For the Linux Host Machine**: **Docker** must be installed and running.
- **For any Client Machines**: **Python 3.10+** is required to run `das-cli`.

---

## Installation

### 1. For Linux Users (Debian-based systems like Ubuntu)

#### Debian Package (Recommended)

```bash
# Set up the repository
sudo bash -c "wget -O - http://45.77.4.33/apt-repo/setup.sh | bash"

# Install the package
sudo apt install das-toolbox

# Verify the installation
das-cli --version
```

> **Note :** Installation an specific version

To install a lower version of `das-cli` than the current version, you can use the following command:

```bash
sudo das-cli update-version --version x.y.z
```

If this command does not work, you can try.

```bash
sudo apt install --only-upgrade --allow-downgrades das-toolbox=x.y.z
```

#### From Source

```bash
# Clone the repository with its submodules
git clone --recurse-submodules https://github.com/singnet/das-toolbox.git
cd das-toolbox

# Create and activate a virtual environment
python3 -m venv env
source env/bin/activate

# Install dependencies
cd das-cli/src
pip3 install -r requirements.txt

# Run the CLI
python3 das_cli.py --version
```

#### Binary Release

Download the precompiled binary for your operating system from the [official releases page](https://github.com/singnet/das-toolbox/releases).

### 2. For MacOS/Windows Users (Remote Execution)

On these operating systems, you can only use `das-cli` from source to act as a remote client.

```bash
# Clone the repository with its submodules
git clone --recurse-submodules https://github.com/singnet/das-toolbox.git
cd das-toolbox

# Create and activate a virtual environment
python -m venv env
env\Scripts\activate.bat

# Install dependencies
cd das-cli/src
pip install -r requirements.txt
pip install windows-curses 

# Run the CLI
python das_cli.py --version
```

---

## Quickstart Guides

This section will guide you through the entire process of configuring `das-cli`, launching the necessary services, and loading your first knowledge base.

> [!NOTE]
>
> The commands in this guide are shown in their most basic form (e.g., `das-cli <command>`). How you run them depends on your setup:
> 
> **Local Execution (Linux Only):** You are running `das-cli` on the same machine that will host the DAS services. You can use the commands exactly as shown.
> **Remote Execution (Any OS):** You are using `das-cli` to control a separate Linux host. This is the **only** option for MacOS and Windows users.
> - For every command, you must append the remote flags you configured in Step 1 (e.g., `--remote --host <your-host> --user <your-user> --key-file <path-to-your-private-key>` or `--remote --host <your-host> --user <your-user> --password <your-password>`).
> - If you installed `das-cli` by running the Python script directly, replace `das-cli` with `python das_cli.py`.
>

### Step 1: Configure DAS CLI

First, you must configure the connection details for the services `das-cli` will manage. This is an interactive process.

> **Note:** If you are running from the script, replace `das-cli` with `python das_cli.py`.

1. **Local Setup (Linux Only)**
    ```bash
    das-cli config set
    ```

2. **Remote Setup (Any OS)**
    Use this if you are on MacOS, Windows, or a Linux machine controlling another Linux host. Choose one of the authentication methods below.

*   **With SSH Key:**
    ```bash
    das-cli config set --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa
    ```

*   **With Password:**
    ```bash
    das-cli config set --remote --host 192.168.0.10 --user ubuntu --password my_p@ssword
    ```

The tool will prompt you for connection details. For a standard local setup, you can press **Enter** to accept the defaults for most settings.

```text
Enter Redis port [40020]: 
Is it a Redis cluster? [y/N]: N
Enter MongoDB port [40021]: 
Enter MongoDB username [admin]: 
Enter MongoDB password [admin]: 
Is it a MongoDB cluster? [y/N]: N
... (You can ignore subsequent settings by pressing Enter)
```

Your settings are now saved, and `das-cli` knows how to connect to the services it will create.

### Step 2: Start the Core Databases

With the configuration in place, start the Redis and MongoDB containers.

> **Note:** If you are running from the script, replace `das-cli` with `python das_cli.py`.

1. **Local Execution**
    ```bash
    das-cli db start
    ```

2. **Remote Execution**
*   **With SSH Key:**
    ```bash
    das-cli db start --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa
    ```
*   **With Password:**
    ```bash
    das-cli db start --remote --host 192.168.0.10 --user ubuntu --password my_p@ssword
    ```

You should see output indicating that the `redis` and `mongodb` containers are `running`.

```bash
Starting Redis service...
Redis has started successfully on port 40020 at localhost, operating under the server user User.
Starting MongoDB service...
MongoDB has started successfully on port 40021 at localhost, operating under the server user User.
```

### Step 3: Load a Knowledge Base

The DAS stores knowledge in the form of MeTTa expressions. These are typically stored in `.metta` files. Let's validate and then load an example file.

> **Note :** You can only upload a file in local execution mode. If you're using **MacOS/Windows**, you'll need to ensure the remote machine already has a knowledge base loaded.

First, create a sample MeTTa file named `animals.metta`:
```metta
(: Similarity Type)
(: Concept Type)
(: Inheritance Type)
(: "human" Concept)
(: "monkey" Concept)
(: "chimp" Concept)
(: "snake" Concept)
(: "earthworm" Concept)
(: "rhino" Concept)
(: "triceratops" Concept)
(: "vine" Concept)
(: "ent" Concept)
(: "mammal" Concept)
(: "animal" Concept)
(: "reptile" Concept)
(: "dinosaur" Concept)
(: "plant" Concept)
(Similarity "human" "monkey")
(Similarity "human" "chimp")
(Similarity "chimp" "monkey")
(Similarity "snake" "earthworm")
(Similarity "rhino" "triceratops")
(Similarity "snake" "vine")
(Similarity "human" "ent")
(Inheritance "human" "mammal")
(Inheritance "monkey" "mammal")
(Inheritance "chimp" "mammal")
(Inheritance "mammal" "animal")
(Inheritance "reptile" "animal")
(Inheritance "snake" "reptile")
(Inheritance "dinosaur" "reptile")
(Inheritance "triceratops" "dinosaur")
(Inheritance "earthworm" "animal")
(Inheritance "rhino" "mammal")
(Inheritance "vine" "plant")
(Inheritance "ent" "plant")
(Similarity "monkey" "human")
(Similarity "chimp" "human")
(Similarity "monkey" "chimp")
(Similarity "earthworm" "snake")
(Similarity "triceratops" "rhino")
(Similarity "vine" "snake")
(Similarity "ent" "human")
```

Now, use `das-cli` to process it:

**Always pass the absolute path to the file.**

> **Note:** If you are running from the script, replace `das-cli` with `python das_cli.py`.

```bash
# 1. Check the file for correct syntax
das-cli metta check $(pwd)/animals.metta

# 2. If the check passes, load it into the database
das-cli metta load $(pwd)/animals.metta
  ```

The knowledge from your file is now stored in the Atomspace.

You can check the atoms in the database using the following command:

1. **Local Execution**
    ```bash
    das-cli db count-atoms
    ```

2. **Remote Execution**
*   **With SSH Key:**
    ```bash
    das-cli db count-atoms --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa
    ```
*   **With Password:**
    ```bash
    das-cli db count-atoms --remote --host 192.168.0.10 --user ubuntu --password my_p@ssword
    ```

### Step 4: Start Agents

Agents are specialized services that perform cognitive functions. They often depend on other services to be running first.

A key example is the **`Query Agent`**, which requires both the databases (started in Step 2) and the **`Attention Broker`** to be active.

First, let's start the `Attention Broker`:

> **Note:** If you are running from the script, replace `das-cli` with `python das_cli.py`.

1. **Local Execution**
    ```bash
    das-cli attention-broker start
    ```
2. **Remote Execution**
*   **With SSH Key:**
    ```bash
    das-cli attention-broker start --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa
    ```
*   **With Password:**
    ```bash
    das-cli attention-broker start --remote --host 192.168.0.10 --user ubuntu --password my_p@ssword
    ```

Now, we can start the `Query Agent`. This command will prompt you to specify a range of network ports for the agent to use.

1. **Local Execution**
    ```bash
    das-cli query-agent start
    ```
2. **Remote Execution**
    ```bash
    das-cli query-agent start --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa

    das-cli query-agent start --remote --host 192.168.0.10 --user ubuntu --password my_p@ssword
    ```

You will then see an interactive prompt. Enter a port range, for example `3000:3100`:

```bash
Enter port range (e.g., 3000:3010): 3000:3100
```

The agent will start and use a port within this range to communicate.

### Step 5: Check the Logs

To confirm that everything is running smoothly, you can inspect the logs of the services.

> **Note:** If you are running from the script, replace `das-cli` with `python das_cli.py`.

1. **Local Execution**
    ```bash
    das-cli logs query-agent
    ```

2. **Remote Execution**
*   **With SSH Key:**
    ```bash
    das-cli logs query-agent --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa
    ```
*   **With Password:**
    ```bash
    das-cli logs query-agent --remote --host 192.168.0.10 --user ubuntu --password my_p@ssword
    ```

**Congratulations!** You now have a basic DAS environment running, complete with databases, a loaded knowledge base, and a running agent.

---

## Updating `das-cli` and Handling Configuration Changes

> [!NOTE]
> The `update-version` command is only available on Debian-based Linux 

To update the tool to the latest version, run the following command with `sudo`:

```bash
sudo das-cli update-version
```

**Important:** After an update, your old configuration file might be deprecated. To avoid issues and use new features, it's best to migrate your settings.

Follow this recommended process to safely update:

1.  **Backup Your Settings**: Before updating, save your current configuration to a text file for reference.
```bash
das-cli config list > my_das_config_backup.txt
```

2.  **Update the Tool**: Run the `update-version` command as shown above.

3.  **Re-configure**: After the update, launch the interactive setup to create an up-to-date configuration file. Use your backup file as a reference to re-enter any custom values.
```bash
das-cli config set
```

This ensures your configuration is compatible with the new version while preserving your custom settings.

---

## Core Funcionality Overview

The Quickstart guide covers the essential workflow for getting a basic environment running. This section provides a broader overview of all major `das-cli` command groups and their specific functions, serving as a quick reference for operations.

### Managing Agents

Agents are specialized services that perform the cognitive work within the DAS. `das-cli` provides a consistent interface to manage their lifecycle. For nearly every agent, you can use the start, stop, and restart subcommands.

The general command pattern is: `das-cli <agent-name>` **[start | stop | restart]**

The available agents are:

| Agent Name           | Description                                           |
|----------------------|-------------------------------------------------------|
| evolution-agent      | Controls the lifecycle of the Evolution Agent service. |
| inference-agent      | Controls the lifecycle of the Inference Agent service. |
| link-creation-agent  | Controls the lifecycle of the Link Creation Agent service.|
| query-agent          | Controls the lifecycle of the Query Agent service.     |

Example: To restart the Inference Agent, you would run:

1. **Local Execution**
    ```bash
    das-cli inference-agent restart
    ```
2. **Remote Execution**
    ```bash
    das-cli inference-agent restart --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa

    das-cli inference-agent restart --remote --host 192.168.0.10 --user ubuntu --password my_p@ssword
    ```

### Managing Databases

This command group controls the core storage components of the DAS (Redis and MongoDB).

- **start | stop | restart:** These subcommands manage the lifecycle of the database containers. start launches them, stop shuts them down, and restart does both in sequence.
- **count-atoms:** A diagnostic tool. After loading a MeTTa file, use this command to connect to the databases and display the total number of atoms stored. This is the best way to verify that your metta load command was successful.

Example:

1. **Local Execution**
    ```bash
    das-cli db count-atoms
    ```
2. **Remote Execution**
    ```bash
    das-cli db count-atoms --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa

    das-cli db count-atoms --remote --host 192.168.0.10 --user ubuntu --password my_p@ssword
    ```

If you want more details about the knowledge base, you can pass the `--verbose` flag.

```bash
das-cli db count-atoms --verbose
das-cli db count-atoms --verbose --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa
```

### Managing MeTTa Files

This group handles the knowledge base files that feed the **DAS**.

- **check**: Validates the syntax of a .metta file without loading it. It is highly recommended to run this before any load operation to catch errors early.
- **load**: Parses a .metta file and imports its atoms into the databases.

### Managing Configuration

This group manages the settings `das-cli` uses to connect to services.

- **set**: Launches the interactive prompt to configure all parameters.
- **list (or ls)**: Displays all the current configuration values in your terminal, which is useful for a quick check without going through the interactive setup.

#### Configurable Parameters

Here is a detailed breakdown of the parameters you can configure. These settings define how `das-cli` connects to and manages the various DAS components.

-   `redis.port`: The network port for the Redis server. Default: `40020`.
-   `redis.cluster`: A boolean (`True`/`False`) indicating if Redis is deployed in a cluster configuration. Default: `False`.
-   `mongodb.port`: The network port for the MongoDB server. Default: `40021`.
-   `mongodb.username`: The username for authenticating with the MongoDB database. Default: `admin`.
-   `mongodb.password`: The password for the specified MongoDB user. Default: `admin`.
-   `mongodb.cluster`: A boolean (`True`/`False`) indicating if MongoDB is deployed in a cluster configuration. Default: `False`.
-   `jupyter_notebook.port`: The network port for accessing the Jupyter Notebook service. Default: `40019`.
-   `attention_broker.port`: The port for the Attention Broker, which manages the flow of information between agents. Default: `40001`.
-   `query_agent.port`: The port for the Query Agent, responsible for handling queries against the Atomspace. Default: `40002`.
-   `link_creation_agent.port`: The port for the Link Creation Agent, which forms new relationships (links) between atoms. Default: `40003`.
-   `inference_agent.port`: The port for the Inference Agent, which performs reasoning and pattern matching. Default: `40004`.
-   `evolution_agent.port`: The port for the Evolution Agent, which manages the long-term growth and maintenance of the knowledge base. Default: `40005`.

### Monitoring with Logs

The logs command is tool for debugging and monitoring the health of your services. You can view the logs for any running service, including all agents and databases.

The general command pattern is: `das-cli logs <service-name>`

Example: To debug a potential issue with Redis, you can view its logs:

1. **Local Execution**
    ```bash
    das-cli logs attention-broker
    ```
2. **Remote Execution**
    ```bash
    das-cli logs attention-broker --remote --host 192.168.0.10 --user ubuntu --key-file ~/.ssh/id_rsa

    das-cli logs attention-broker --remote --host 192.168.0.10 --user ubuntu --password my_p@ssword
    ```

---
