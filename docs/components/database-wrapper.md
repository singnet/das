# DAS Database Adapter

The DAS Database Adapter is a high-performance modular system designed to map data from SQL and NOSQL databases into Atoms. The system uses a Hexagonal Architecture to ensure decoupling and scalability across various data processing needs.

## Features

1. Data Mapping: Converts raw data from databases into domain-specific Atoms.
2. Process Mapped Atoms: Supports multiple processors, including:
   - RedisMongoProcessor: Saves data to AtomDB of type Redis/MongoDB.
   - DasNodeProcessor: Sends data to DAS node network for processing.
3. Observer Pattern (WIP): An Observer module listens for changes in the database (e.g., new tables, updated rows, schema changes) and triggers automatic data mapping into Atoms.

## Installation

> Before you start, make sure you have [Python](https://www.python.org/) >= 3.10 and [Pip](https://pypi.org/project/pip/) installed on your system.
    
1.  Clone the project repository:
    
```bash
$ git clone git@github.com:singnet/das-database-adapter.git
$ cd das-database-adapter
```

2. Create virtual environment and activate (bash/zsh).[For other plataforms](https://docs.python.org/3.10/library/venv.html#creating-virtual-environments)
    
```bash
$ python -m venv /path/to/venv
$ source /path/to/venv/bin/activate
``` 
    
3.  Install dependencies:
    
```bash
pip install -r requirements.txt
```

## Usage

The main goal of the project is to map data from a PostgreSQL database to Atoms using the DAS Node network. It contains two main components:

1. Server: A service that receives and stores the atoms in AtomDB.
2. Client: A client that maps the data in PostgreSQL and sends it to the server for saving.

**NOTE: Usage via das-cli**

Usage via das-cli is recommended. Details can be found here [das-toolbox](https://github.com/singnet/das-toolbox?tab=readme-ov-file#examples)

### Starting the server

1. Build
```bash
$ make server-build
```

2. Start
```bash
$ make server-up ARGS="--server-id localhost:30100 --mongo-hostname localhost --mongo-port 27017 --mongo-username admin --mongo-password 1234 --redis-hostname localhost --redis-port 6379"
```

#### Parameters

- `--server-id`: The server ID, including the hostname and port. Example: localhost:30100.
- `--mongo-hostname`: MongoDB hostname.
- `--mongo-port`: MongoDB port.
- `--mongo-username`: MongoDB username.
- `--mongo-password`: MongoDB password.
- `--redis-hostname`: Redis hostname.
- `--redis-port`: Redis port. 
- `--redis-username`: Redis username. Optional. Default is None
- `--redis-password`: Redis password. Optional. Default is None

### Starting the client

1. Build
```bash
$ make client-build
```

2. Start
```bash
$ make client-up ARGS="--node-id localhost:30200 --server-id localhost:30100 --postgres-hostname 123.45.67.8 --postgres-port 5432 --secrets-file secrets.ini --context /path/to/context.txt"
```

#### Configuration

The `secrets.ini` file must be located at the root of the project and should contain the database credentials

```ini
[postgres]
username = admin-postgres
password = 5678

```

#### Parameters

- `--node-id`: The client ID, including the hostname and port. Example: localhost:30200.
- `--server-id`: The server ID to which the client will send data.
- `--postgres-hostname`: PostgreSQL server hostname where the data is stored.
- `--postgres-port`: PostgreSQL server port.
- `--postgres-database`: Optional. Default is `postgres`.
- `--secrets-file`: Path to the secrets.ini file containing the credentials.
- `--context`: Path to the context file containing information necessary for mapping.


### Stop server and client

1. Server
```bash
$ make server-down PORT=1234
```

1. Client
```bash
$ make client-down PORT=5678
```

### Context

- The context file must be a .txt file, and it should follow this structure:

1. **Structure 1**
```txt
TABLE_ID <CLAUSE_1> <CLAUSE_2> <CLAUSE_3> ...
```

`TABLE_ID` represents the fully qualified name of a table in the format `schema_name.table_name`, followed by one or more WHERE clauses. These WHERE clauses will be used to construct a SQL query in the following format:

```sql
SELECT * FROM TABLE_ID WHERE CLAUSE_1 AND CLAUSE_2 AND ...
```

Each WHERE clause should be enclosed in <>, and single spaces should be used to separate TABLE_ID and each WHERE clause.

Example:
```txt
public.feature <name IN ('Abd-B', 'Var')> <is_obsolete=false>
```


2. **Structure 2**
```txt
SQL TABLE_NAME_REFERENCE <query SQL>
```

`TABLE_NAME_REFERENCE` represents the name of the table created by the SQL query, formatted as ```SQL TABLE_NAME_REFERENCE <SQL_QUERY>```. The table name can be any identifier you choose, followed by a space and then the SQL query. The query should be enclosed within **<>** and must use aliases for each column in the format `TABLE_ID__COLUMN_NAME`. This structure allows for consistent naming and mapping when the query results are used.

Example:
```txt
SQL table_experiment_1 <SELECT grp.uniquename as public_grp__uniquename, grp.name as public_grp__name
FROM grp
JOIN grpmember ON grpmember.grp_id = grp.grp_id
JOIN feature_grpmember ON feature_grpmember.grpmember_id = grpmember.grpmember_id
JOIN feature ON feature.feature_id = feature_grpmember.feature_id
WHERE feature.name = 'Abd-B' AND feature.is_obsolete = 'f';>
```

**NOTE:** The context file can have multiple lines and each line must follow either structure 1 or structure 2
