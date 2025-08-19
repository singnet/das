# DatabaseWrapper - User's Guide

This guide will show you what DatabaseWrapper is, how it works, and walk you through using it, enabling you to map data from a PostgreSQL database into MeTTa expressions and interact with the DAS ecosystem. The mapping logic used to convert SQL data to MeTTa expressions can be seen in the diagram below.

<p align="center">
<img src="../docs/assets/mapping.jpg" width="400"/>
</p>

---

## Table of Contents

- [DatabaseWrapper - User's Guide](#databasewrapper---users-guide)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Prerequisites](#prerequisites)
  - [Quickstart Example](#quickstart-example)
    - [1. Clone the Repository](#1-clone-the-repository)
    - [2. Prepare Secrets and Context](#2-prepare-secrets-and-context)
    - [3. Build and Run](#3-build-and-run)
  - [Step-by-Step Usage](#step-by-step-usage)
    - [1. Clone the Repository](#1-clone-the-repository-1)
    - [2. Configure the Environment](#2-configure-the-environment)
    - [3. Prepare Secrets and Context](#3-prepare-secrets-and-context)
    - [4. Build and Run the Adapter](#4-build-and-run-the-adapter)
  - [Understanding the Context File](#understanding-the-context-file)
    - [Context Structures](#context-structures)
      - [**Structure 1: Table Filter**](#structure-1-table-filter)
      - [**Structure 2: Subquery Table \[Unstable\]**](#structure-2-subquery-table-unstable)
    - [Writing Your Own Context](#writing-your-own-context)
  - [Load the mapped data into the DAS](#load-the-mapped-data-into-the-das)
  - [Troubleshooting and Tips](#troubleshooting-and-tips)

---

## Introduction

The DAS Database Adapter is a high-performance modular system designed to map data from SQL and NOSQL databases into Atoms. The system uses a Hexagonal Architecture to ensure decoupling and scalability across various data processing needs. 

You should use the [`das-cli`](https://github.com/singnet/das-toolbox) to set up, configure, and interact with the DAS system. *Direct builds of the repository are not recommended for end users.*

---

## Prerequisites

- **Docker** installed and running.
- **das-cli** installed ([see das-toolbox](https://github.com/singnet/das-toolbox)).

---

## Quickstart Example

This example maps data from the [FlyBase public PostgreSQL database](https://flybase.github.io/docs/chado/#public-database) using the provided context and secrets.

### 1. Clone the Repository

```bash
git clone git@github.com:singnet/das-database-adapter.git
cd das-database-adapter
```

### 2. Prepare Secrets and Context

- **secrets.ini**: Use the provided example at. `./examples/secrets.ini`:

    ```ini
    [postgres]
    username=flybase
    password=
    ```

- **context.txt**: Use the provided example at `./examples/context.txt`:

    ```txt
    public.feature -[residues,seqlen,md5checksum] <uniquename LIKE 'FBgn%'><is_obsolete=false>
    ```

### 3. Build and Run

```bash
make build
make run host=chado.flybase.org port=5432 db=flybase secrets=$(pwd)/examples/secrets.ini context=$(pwd)/examples/context.txt output=$(pwd)/
```

If everything is configured correctly, the adapter will connect to the remote database, map the data as described in the context, and process it into MeTTa files. You should see the generated output files in the current directory and terminal output similar to the one below:

<p align="center">
<img src="../docs/assets/mapping_output.png" width="400"/>
</p>

---

## Step-by-Step Usage

### 1. Clone the Repository

Follow the Quickstart step above.

### 2. Configure the Environment

Ensure Docker is running and `das-cli` is installed. All interactions should be through `das-cli` for ease of use and future compatibility.

### 3. Prepare Secrets and Context

- **Secrets file**: Contains your DB credentials. See above.
- **Context file**: Describes exactly what data to extract and how.

### 4. Build and Run the Adapter

- Use the provided Make targets (`make build`, `make run`).
- Pass all runtime arguments as shown above.

---

## Understanding the Context File

The context file is the core of your data mapping. It precisely defines what data is fetched and how.

### Context Structures

There are two supported structures for context lines:

#### **Structure 1: Table Filter**

```txt
schema_name.table_name -[columns_to_exclude] <CLAUSE_1> <CLAUSE_2> ...
```

- `schema_name.table_name`: Fully qualified table name.
- `columns_to_exclude`: A comma-separated list of columns to exclude from the mapping.
- Each `<CLAUSE>`: A SQL WHERE clause (without `WHERE` keyword).
- Clauses are combined with `AND`.

**Example:**
```txt
public.feature -[residues,md5checksum] <name IN ('Abd-B', 'Var')> <is_obsolete=false>
```

This extracts all rows from `public.feature` where `name` is `'Abd-B'` or `'Var'` and `is_obsolete` is `false` while skipping the `residues` and `md5checksum` columns.

#### **Structure 2: Subquery Table [Unstable]**

```txt
SQL custom_table_name <SQL_QUERY>
```

- `custom_table_name`: Any identifier you assign to this data set.
- `<SQL_QUERY>`: An SQL query enclosed in `<>`. Each column **must** use an alias in the form `schema_table__column`.

**Example:**
```txt
SQL table_experiment_1 <SELECT grp.uniquename as public_grp__uniquename, grp.name as public_grp__name
FROM grp
JOIN grpmember ON grpmember.grp_id = grp.grp_id
JOIN feature_grpmember ON feature_grpmember.grpmember_id = grpmember.grpmember_id
JOIN feature ON feature.feature_id = feature_grpmember.feature_id
WHERE feature.name = 'Abd-B' AND feature.is_obsolete = 'f';>
```

**Notes:**
- You can mix both types in a single context file.
- Each line is processed independently.

### Writing Your Own Context

1. **Identify the tables and data you want to extract.**
2. **Decide if a simple filter (Structure 1) or a custom query (Structure 2) is needed.**
3. **Write one context line per data set you want to extract.**
4. **Always use correct schema and table names, and valid SQL syntax for clauses and queries.**

---

## Load the mapped data into the DAS

Once the data mapping is complete, you can load it into DAS using the `das-cli metta load` command.
Make sure das-cli is configured correctly, then run:

1. **das-cli db start**: This will start the database in your environment.
2. **das-cli metta load /absolute/path/to/metta_file.metta**: This will load the specified MeTTa file into DAS.

---
## Troubleshooting and Tips

- **Connection errors?** Double-check your `secrets.ini` and database host/port.
- **No data mapped?** Review your context file for typos or query logic.
- **Custom context not working?** Make sure your SQL queries use the proper aliasing format as shown.

---
