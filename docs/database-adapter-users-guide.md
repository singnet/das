# Database Adapter - User's Guide

This guide will show you what Database Adapter is, how it works, and walk you through using it, enabling you to map data from a PostgreSQL database and load it into the DAS. The mapping logic used to convert SQL data to MeTTa expressions can be seen in the diagram below.

<p align="center">
<img src="../docs/assets/mapping.jpg" width="400"/>
</p>

---

## Table of Contents

- [Database Adapter - User's Guide](#database-adapter---users-guide)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Prerequisites](#prerequisites)
  - [Quickstart Example](#quickstart-example)
    - [1. Configure the DAS stack](#1-configure-the-das-stack)
      - [1. Install das-cli and start the DAS services](#1-install-das-cli-and-start-the-das-services)
      - [2. Clone the DAS repository and setting up the environment](#2-clone-the-das-repository-and-setting-up-the-environment)
      - [3. Build DAS components](#3-build-das-components)
      - [4. Run the AtomDBBroker](#4-run-the-atomdbbroker)
    - [2. Build and run the Database Adapter](#2-build-and-run-the-database-adapter)
      - [1. Clone the DAS Database Adapter repository](#1-clone-the-das-database-adapter-repository)
      - [2. Prepare Secrets and Context](#2-prepare-secrets-and-context)
      - [3. Build and run the Adapter](#3-build-and-run-the-adapter)
  - [Understanding the Context File](#understanding-the-context-file)
    - [Context Structures](#context-structures)
      - [**Structure 1: Table Filter (Recommended)**](#structure-1-table-filter-recommended)
      - [**Structure 2: Subquery Table (WIP)**](#structure-2-subquery-table-wip)
    - [Writing Your Own Context File](#writing-your-own-context-file)
  - [Troubleshooting and Tips](#troubleshooting-and-tips)

---

## Introduction

The DAS Database Adapter is a modular system designed to map data from SQL databases into Atoms (MeTTa symbols and expressions).
The main workflow involves cloning the [repository](https://github.com/singnet/das-database-adapter), configuring a context file to define the data mapping, and running the adapter passing the necessary parameters.

---

## Prerequisites

Before you begin, ensure you have the following tools installed and running:

- **Docker:** Essential for building and running the adapter in a containerized environment.
- **das-cli:** The DAS command-line interface. You can find installation instructions in the [das-cli User's Guide](das-cli-users-guide.md).

---

## Quickstart Example

This example maps data from the [FlyBase public PostgreSQL database](https://flybase.github.io/docs/chado/#public-database) using the provided context and secrets.

> [!NOTE]
> Currently, it is only possible to use the Database Adapter with scripts directly from the repository.
> In the future, a version of the Adapter will be integrated into `das-cli` to facilitate its use.

For the mapping and loading of data into DAS to work correctly, you need to follow these steps:

### 1. Configure the DAS stack

#### 1. Install das-cli and start the DAS services

Follow the instructions in the [das-cli User's Guide](das-cli-users-guide.md) to install `das-cli`.

- Configuring your DAS environment

```bash
das-cli config set
```

- Starting an AtomDB

  ```bash
  das-cli db start
  ```

#### 2. Clone the DAS repository and setting up the environment

  ```bash
  git clone https://github.com/singnet/das.git
  cd das
  ```

  You need to create a `.env` file in the `das` directory with the following content:

  ```bash
  DAS_REDIS_HOSTNAME=0.0.0.0
  DAS_REDIS_PORT=29000
  DAS_USE_REDIS_CLUSTER=False
  DAS_MONGODB_HOSTNAME=0.0.0.0
  DAS_MONGODB_PORT=28000
  DAS_MONGODB_USERNAME=dbadmin
  DAS_MONGODB_PASSWORD=dassecret
  ```

#### 3. Build DAS components

  ```bash
  make build-all
  ```

#### 4. Run the AtomDBBroker

  ```bash
  make run-busnode OPTIONS="--service=atomdb-broker --endpoint=localhost:40007 --ports-range=25000:26000"
  ```

  **IMPORTANT:** The endpoint `<ip:port>` of the AtomDBBroker is used as `peer_id` when running the adapter.

  You will see this output in the terminal:

  ```bash
  2025-12-03 13:55:02 | [INFO] | Starting service: atomdb-broker
  2025-12-03 13:55:02 | [INFO] | Using default MongoDB chunk size: 1000
  2025-12-03 13:55:02 | [INFO] | Connected to MongoDB at 0.0.0.0:40021
  2025-12-03 13:55:02 | [INFO] | WARNING: No pattern_index_schema found, all possible patterns will be created during link insertion!
  2025-12-03 13:55:02 | [INFO] | Connected to (NON-CLUSTER) Redis at 0.0.0.0:40020
  2025-12-03 13:55:02 | [INFO] | BUS static initialization
  2025-12-03 13:55:02 | [INFO] | BUS command: <atomdb>
  2025-12-03 13:55:02 | [INFO] | BUS command: <context>
  2025-12-03 13:55:02 | [INFO] | BUS command: <inference>
  2025-12-03 13:55:02 | [INFO] | BUS command: <link_creation>
  2025-12-03 13:55:02 | [INFO] | BUS command: <pattern_matching_query>
  2025-12-03 13:55:02 | [INFO] | BUS command: <query_evolution>
  2025-12-03 13:55:02 | [INFO] | Port range: [25000 : 26000]
  2025-12-03 13:55:02 | [INFO] | New BUS on: localhost:40007
  2025-12-03 13:55:02 | [INFO] | SynchronousGRPC listening on localhost:40007
  2025-12-03 13:55:03 | [INFO] | Registering processor for service: atomdb-broker
  2025-12-03 13:55:03 | [INFO] | Processor registered for service: atomdb-broker
  2025-12-03 13:55:03 | [INFO] | BUS node localhost:40007 is taking ownership of command atomdb
  ```

### 2. Build and run the Database Adapter

#### 1. Clone the DAS Database Adapter repository

```bash
git clone https://github.com/singnet/das-database-adapter.git
cd das-database-adapter
```

#### 2. Prepare Secrets and Context

Before running the adapter, you need to set up two configuration files: `secrets.ini` (with SQL database credentials) and `context.txt`.

- **secrets.ini**: Use the provided example at `./examples/secrets.ini`. It contains the database credentials:

```ini
[postgres]
username=flybase
password=
```

- **context.txt**: Use the provided example at `./examples/context.txt`. It defines what data to extract:

```txt
public.feature -[residues,seqlen,md5checksum] <<uniquename LIKE 'FBgn%'>><<is_obsolete=false>>
```

**NOTE:** The context files are better explained in the [Understanding the Context File](#understanding-the-context-file) section.

#### 3. Build and run the Adapter

Build the adapter's Docker image (in the das-database-adapter directory)

```bash
make build

make run \
  host=chado.flybase.org \
  port=5432 \
  db=flybase \
  secrets=$(pwd)/examples/secrets.ini \
  context=$(pwd)/examples/context.txt \
  peer_id=localhost:40007
```

If everything is configured correctly, the adapter will connect to the remote database, map the data as described in the context, and send the Atoms to AtomDB. You should see the terminal output similar to the one below:

```bash
  ==> Script starting...
  Using SQL2ATOMS mapper type.
  2025-12-03 14:06:44 | [INFO] | Port range: [50000 : 50999]
  2025-12-03 14:06:44 | [INFO] | 0.0.0.0:9999 is issuing BUS command <atomdb>
  2025-12-03 14:06:44 | [INFO] | BUS node 0.0.0.0:9999 is routing command <atomdb> to localhost:40007
  2025-12-03 14:06:44 | [INFO] | Remote command: <node_joined_network> arrived at DistributedAlgorithmNodeServicer 0.0.0.0:50000
  2025-12-03 14:06:51 | [INFO] | Mapped 879/87882 (1%) rows from the public.cvterm table
  2025-12-03 14:06:52 | [INFO] | Mapped 1758/87882 (2%) rows from the public.cvterm table
  2025-12-03 14:06:53 | [INFO] | Mapped 2637/87882 (3%) rows from the public.cvterm table
  2025-12-03 14:06:53 | [INFO] | Mapped 3516/87882 (4%) rows from the public.cvterm table
  2025-12-03 14:06:54 | [INFO] | Mapped 4395/87882 (5%) rows from the public.cvterm table
  2025-12-03 14:06:55 | [INFO] | Mapped 5273/87882 (6%) rows from the public.cvterm table
  2025-12-03 14:06:55 | [INFO] | Mapped 6152/87882 (7%) rows from the public.cvterm table
  2025-12-03 14:06:56 | [INFO] | Mapped 7031/87882 (8%) rows from the public.cvterm table
  2025-12-03 14:06:57 | [INFO] | Mapped 7910/87882 (9%) rows from the public.cvterm table
  2025-12-03 14:06:57 | [INFO] | Mapped 8789/87882 (10%) rows from the public.cvterm table
```

**After these steps, you will have your Postgres database properly loaded into DAS.**

**Tip:** You can monitor the number of Atoms in the AtomDB using the following command:

```bash
das-cli db count-atoms
```

---

## Understanding the Context File

The context file is the core of your data mapping. It precisely defines what data is fetched and how.

### Context Structures

There are two supported structures for contexts. Each context in the file is processed independently and ends with `>>`.

#### **Structure 1: Table Filter (Recommended)**

```txt
schema_name.table_name -[columns_to_exclude] <<CLAUSE_1>> <<CLAUSE_2>> ...
```

- `schema_name.table_name`: Fully qualified table name.
- `columns_to_exclude`: A comma-separated list of columns to exclude from the mapping. Don't include spaces.
- Each `<<CLAUSE>>`: A SQL WHERE clause (without `WHERE` keyword).
- Clauses are combined with `AND` operator.

**NOTE:** If you don't want to exclude any columns, you must pass `-[]`.

**Example:**

```txt
public.feature -[residues,seqlen,md5checksum] <<is_obsolete=false>><<uniquename LIKE 'FBgn%'>> 
public.feature -[residues,seqlen,md5checksum] <<is_obsolete=false>><<type_id IN (1179)>>
public.cvterm -[definition] <<is_obsolete=0>>
public.feature_cvterm -[] <<is_not=false>><<feature_id not IN ('11382573','11387545')>>
```

This extracts:
- All rows from `public.feature` where `uniquename` starts with `'FBgn'` and `is_obsolete` is `false` plus all the rows where `type_id` is `1179` and `is_obsolete` is `false`, while skipping the `residues`, `seqlen`, and `md5checksum` columns
- All rows from `public.cvterm` where `is_obsolete` is `0`, while skipping the `definition` column.
- All rows from `public.feature_cvterm` where `is_not` is `false` and `feature_id` is not in the specified list, *without skipping any columns*.

#### **Structure 2: Subquery Table (WIP)**

```txt
SQL virtual_table_name <<SQL_QUERY>>
```

- `virtual_table_name`: Any identifier you assign to this data set.
- `<SQL_QUERY>`: An SQL query enclosed in `<<>>`.

**IMPORTANT:** Mandatory rules for the SQL query:

- Explicitly define the columns with aliases in the format `table_schema__column` (so that the adapter knows which table/column each value belongs to)
- Include the primary key of each table involved, also with the alias in the same pattern.

**Example 1:**
```txt
SQL my_grp <<
SELECT
grp.grp_id as public_grp__grp_id,
grp.uniquename as public_grp__uniquename,
grp.name as public_grp__name
FROM grp
JOIN grpmember ON grpmember.grp_id = grp.grp_id
JOIN feature_grpmember ON feature_grpmember.grpmember_id = grpmember.grpmember_id
JOIN feature ON feature.feature_id = feature_grpmember.feature_id
WHERE feature.name = 'Abd-B' AND feature.is_obsolete = 'f';>>
```

Is this example, the adapter will map the following data:

```txt
public_grp__grp_id|public_grp__uniquename|public_grp__name|
------------------+----------------------+----------------+
327               |FBgg0000366           |BX-C            |
708               |FBgg0000747           |HOXL            |
324               |FBgg0000363           |HOX-C           |
705               |FBgg0000744           |HBTF            |
707               |FBgg0000746           |HTH             |
706               |FBgg0000745           |TFS             |
```

**Example 2:**
```txt
SQL gene_expr <<
SELECT
feature.feature_id as public_feature__feature_id,
feature.name as public_feature__name,
feature.uniquename as public_feature__uniquename,
library.library_id as public_library__library_id,
library.uniquename as public_library__uniquename,
library_featureprop.library_featureprop_id as public_library_featureprop__library_featureprop_id,
library_featureprop.value as public_library_featureprop__value
FROM
feature
join library_feature on library_feature.feature_id = feature.feature_id
join library on library.library_id = library_feature.library_id left
join library_featureprop on library_featureprop.library_feature_id = library_feature.library_feature_id
WHERE
library_featureprop.type_id = '151505' AND
library.uniquename in (
    'FBlc0003850', 'FBlc0003851', 'FBlc0003852', 'FBlc0003853', 'FBlc0003854',
    'FBlc0003855', 'FBlc0003856', 'FBlc0003857', 'FBlc0003858', 'FBlc0003859',
    'FBlc0003860', 'FBlc0003861', 'FBlc0003862', 'FBlc0003863', 'FBlc0003864',
    'FBlc0003865', 'FBlc0003866', 'FBlc0003867', 'FBlc0003868', 'FBlc0003869',
    'FBlc0003870', 'FBlc0003871', 'FBlc0003872', 'FBlc0003873', 'FBlc0003874');>>
```

---

### Writing Your Own Context File

1.  **Identify** the tables and data you want to extract.
2.  **Decide** if a simple filter (Structure 1) is sufficient or if a custom query (Structure 2) is needed.
3.  **Write** one context line per dataset you want to extract.
4.  **Always use** complete schema and table names and valid SQL syntax.

---

## Troubleshooting and Tips

-   **Connection errors?** Double-check your `secrets.ini` and the database host/port parameters in the `make run` command.
-   **No data mapped?** Review your `context.txt` for typos, incorrect table/column names, or query logic.
-   **Custom context not working?** Check that your SQL query uses the `schema_table__column` alias format correctly. Preferably use Structure 1.

---
