# Hyperon DAS AtomDB

Persistence layer for Distributed AtomSpace

## Installation

> Before you start, make sure you have [Python](https://www.python.org/) >= 3.10 and [Pip](https://pypi.org/project/pip/) installed on your system.

#### This package is deployed on [Pypi](https://pypi.org/project/hyperon-das-atomdb/). If you want, you can install using the pip command

```
pip install hyperon-das-atomdb
```

## Build and Install

You can also build the package locally by running the following command from the project root:

```bash
make build
```

This will generate the binaries for all components in the `das/src/bin` directory, including the wheel for the hyperon-das-atomdb.

Optionally, you can activate a virtual environment using `venv` or any other tool to create a Python virtual environment:

```bash
python3 -m venv env
```

To activate the virtual environment:

```bash
source env/bin/activate
```

After activating the virtual environment, you can install the package using the following command:

```bash
pip install src/bin/hyperon_das_atomdb-0.8.11-py3-none-any.whl
```

## Environment Variables

You must have the following variables set in your environment with their respective values:

```
DAS_MONGODB_HOSTNAME=172.17.0.2
DAS_MONGODB_PORT=27017
DAS_MONGODB_USERNAME=mongo
DAS_MONGODB_PASSWORD=mongo
DAS_MONGODB_TLS_CA_FILE=global-bundle.pem       [optional]
DAS_REDIS_HOSTNAME=127.0.0.1
DAS_REDIS_PORT=6379
DAS_REDIS_USERNAME=admin                        [optional]
DAS_REDIS_PASSWORD=admin                        [optional]
DAS_USE_REDIS_CLUSTER=false                     [optional - default: true]
DAS_USE_REDIS_SSL=false                         [optional - default: true]
```

## Usage

**1 - Redis and MongoDB**

- You must have Redis and MongoDB running in your environment
- To initialize the databases you must pass the parameters with the necessary values. Otherwise, default values will be used. See below which parameters it is possible to pass and their respective default values:

```python
from hyperon_das_atomdb.adapters import RedisMongoDB

redis_mongo_db = RedisMongoDB(
        mongo_hostname='localhost',
        mongo_port=27017,
        mongo_username='mongo',
        mongo_password='mongo',
        mongo_tls_ca_file=None,
        redis_hostname='localhost',
        redis_port=6379,
        redis_username=None,
        redis_password=None,
        redis_cluster=True,
        redis_ssl=True,
)
```

**2 - In Memory DB**

```python
from hyperon_das_atomdb.adapters import InMemoryDB

in_memory_db = InMemoryDB()
```
