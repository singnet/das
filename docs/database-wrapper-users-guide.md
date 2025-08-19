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
    - [4. Check and Output](#4-check-and-output)
  - [Step-by-Step Usage](#step-by-step-usage)
  - [Understanding the Context File](#understanding-the-context-file)
    - [Context Structures](#context-structures)
      - [**Structure 1: Table Filter (Recommended)**](#structure-1-table-filter-recommended)
      - [**Structure 2: Subquery Table (Unstable)**](#structure-2-subquery-table-unstable)
    - [Writing Your Own Context](#writing-your-own-context)
  - [Load the mapped data into the DAS](#load-the-mapped-data-into-the-das)
  - [Troubleshooting and Tips](#troubleshooting-and-tips)

---

## Introduction

The DAS Database Adapter is a high-performance modular system designed to map data from SQL databases into Atoms (MeTTa expressions).
The primary workflow involves cloning this repository, configuring a context file to define the data mapping, and running the adapter to generate `.metta` files.

These `.metta` files can then be loaded into the Distributed AtomSpace (DAS) using the [`das-cli`](https://github.com/singnet/das-toolbox), the command-line tool for interacting with the DAS ecosystem.

---

## Prerequisites

Before you begin, ensure you have the following tools installed and running:

- **Docker** Essential for building and running the adapter in a containerized environment.
- **das-cli** The DAS command-line interface. While not used to *generate* MeTTa files with this adapter, it is **required for the next step**: loading the data into DAS. ([See das-toolbox](https://github.com/singnet/das-toolbox) for installation).

---

## Quickstart Example

This example maps data from the [FlyBase public PostgreSQL database](https://flybase.github.io/docs/chado/#public-database) using the provided context and secrets.

### 1. Clone the Repository

```bash
git clone git@github.com:singnet/das-database-adapter.git
cd das-database-adapter
```

### 2. Prepare Secrets and Context

- **secrets.ini**: Use the provided example at. `./examples/secrets.ini`. It contains the database credentials:

    ```ini
    [postgres]
    username=flybase
    password=
    ```

- **context.txt**: Use the provided example at `./examples/context.txt`. It defines what data to extract:

    ```txt
    public.feature -[residues,seqlen,md5checksum] <uniquename LIKE 'FBgn%'><is_obsolete=false>
    ```

### 3. Build and Run

Use the `make` commands to build the Docker image and run the adapter. Pass the connection parameters and paths to your configuration files.

```bash
# Build the adapter's Docker image
make build

# Run the adapter
make run host=chado.flybase.org port=5432 db=flybase secrets=$(pwd)/examples/secrets.ini context=$(pwd)/examples/context.txt output=$(pwd)/
```

If everything is configured correctly, the adapter will connect to the remote database, map the data as described in the context, and process it into MeTTa files in the specified output directory. You should see the generated output files in the current directory and terminal output similar to the one below:

<p align="center">
<img src="../docs/assets/mapping_output.png" width="400"/>
</p>

### 4. Check and Output

After execution, several directories will be created, where each directory represents a table that was mapped according to the passed context and each directory will contain one or more `.metta` files. This files contains your SQL data translated into the MeTTa format. Each record in the table becomes a MeTTa expression, as shown in the [image](#databasewrapper---users-guide). See an example of a generated .metta file bellow:


```metta
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "100182789")))
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "100182790")))
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "100182979")))
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "100182980")))
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "1005074")))
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "100572913")))
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "1013")))
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "101601525")))
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "101601526")))
(EVALUATION (PREDICATE (public.feature public.feature.is_analysis "False")) (CONCEPT (public.feature "101601527")))
```
---

## Step-by-Step Usage

The main workflow will always follow the Quickstart steps:

1.  **Clone the Repository**: Get the adapter's code.
2.  **Configure the Environment**: Ensure Docker is running.
3.  **Prepare Files**: Create or edit your `secrets.ini` with your database credentials and your `context.txt` to define the data extraction.
4.  **Build and Run**: Use `make build` and `make run` with the correct parameters for your database and files.

---

## Understanding the Context File

The context file is the core of your data mapping. It precisely defines what data is fetched and how.

### Context Structures

There are two supported structures for context lines. Each line in the file is processed independently.

#### **Structure 1: Table Filter (Recommended)**

```txt
schema_name.table_name -[columns_to_exclude] <CLAUSE_1> <CLAUSE_2> ...
```

- `schema_name.table_name`: Fully qualified table name.
- `columns_to_exclude`: A comma-separated list of columns to exclude from the mapping.
- Each `<CLAUSE>`: A SQL WHERE clause (without `WHERE` keyword).
- Clauses are combined with `AND` operator.

**Example:**
```txt
public.feature -[residues,md5checksum] <name IN ('Abd-B', 'Var')> <is_obsolete=false>
```

This extracts all rows from `public.feature` where `name` is `'Abd-B'` or `'Var'` and `is_obsolete` is `false`, while skipping the `residues` and `md5checksum` columns.

#### **Structure 2: Subquery Table (Unstable)**

**Warning:** This feature is experimental and may not work as expected. For reliable mappings, please prefer **Structure 1**.

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
**Note:** The `schema_table__column` alias convention is mandatory for the adapter to track the origin of each data field.

---

### Writing Your Own Context

1.  **Identify** the tables and data you want to extract.
2.  **Decide** if a simple filter (Structure 1) is sufficient or if a custom query (Structure 2) is needed, remembering that Structure 1 is recommended.
3.  **Write** one context line per dataset you want to extract.
4.  **Always use** correct schema and table names and valid SQL syntax.

---

## Load the mapped data into the DAS

Once the data mapping is complete and the `.metta` files have been generated, you can load them into DAS using the `das-cli`, which you should already have installed as a prerequisite.

1.  **`das-cli db start`**: This command starts the DAS internal database, which will store your data.
2.  **`das-cli metta load /absolute/path/to/your/file.metta`**: This command reads the specified `.metta` file and inserts the data into the AtomSpace, making it available for querying and processing within the DAS ecosystem.

---
## Troubleshooting and Tips

-   **Connection errors?** Double-check your `secrets.ini` and the database host/port parameters in the `make run` command.
-   **No data mapped?** Review your `context.txt` for typos, incorrect table/column names, or query logic.
-   **Custom context not working?** Remember that Structure 2 (Subquery) is unstable. Check that your SQL query uses the `schema_table__column` alias format correctly. Preferably use Structure 1.

---
