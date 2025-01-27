# Hyperon DAS AtomDB

Persistence layer for Distributed AtomSpace

## Installation

This package requires:
[python](https://www.python.org/) >= 3.10 to run.

#### This package is deployed on [Pypi](https://pypi.org/project/hyperon-das/). If you want, you can install using the pip command

```
pip install hyperon-das-atomdb
```

#### If you want to run it without installing it by pip, you can follow the following approach

We use the [Poetry](https://python-poetry.org/) package to manage project dependencies and other things. So, if you have Poetry on your machine, you can run the commands below to prepare your environment

**1. poetry install**

**2. poetry shell** (activate virtual environment)

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
DAS_USE_REDIS_CLUSTER=false                     [default: true]
DAS_USE_REDIS_SSL=false                         [default: true]
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

## Pre-Commit Setup

Before pushing your changes, it's recommended to set up pre-commit to run automated tests locally. Run the following command (needs to be done once):

```bash
pre-commit install
```

## Tests

You can ran the command below to execute the unit tests

```bash
make unit-tests
```

## Documentation References

[Repositories documentation](https://docs.google.com/document/d/1njmP_oXw_0FLwoXY5ttGBMFGV2n60-ugAltWIuoQO10/)

## Release Notes

[DAS AtomDB Releases](https://github.com/singnet/das-atom-db/releases)
