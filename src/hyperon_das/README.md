# Hyperon DAS

A data manipulation API for Distributed Atomspace (DAS). It allows queries with pattern matching capabilities and traversal of the Atomspace hypergraph.

## Installation

> Before you start, make sure you have [Python](https://www.python.org/) >= 3.10 and [Pip](https://pypi.org/project/pip/) installed on your system.

#### This package is deployed on [Pypi](https://pypi.org/project/hyperon-das/). If you want, you can install using the pip command

```bash
pip install hyperon-das
```

## Build and Install

You can also build the package locally by running the following command from the project root:

```bash
make build-all
```

This will generate the binaries for all components in the `das/src/bin` directory, including the wheel for the hyperon-das.

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
