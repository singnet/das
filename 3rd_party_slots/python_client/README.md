# Python Bus Client

A Python-based client that communicates with a remote service bus via gRPC. Provides utilities for building, packaging, and executing a pattern matching query against a service bus.

## Setup

Clone the repo:

```bash
git clone https://github.com/singnet/das.git
cd das/3rd_party_slots/python_client
```

## Installation

Prerequisites

To build and install the client, you need the following tools:

1. Poetry for dependency management.
2. Docker for building the client (recommended for consistency).

Additionally, ensure you have grpcio-tools installed in your Poetry environment for gRPC stub generation. This is typically included as a development dependency in pyproject.toml.

### Building with Docker (Recommended)

This creates a Docker image tagged as python-client and generates a wheel file in the dist/ directory.

```bash
make build

pip install dist/python_client-0.1.0*.whl
```

### Building locally

If you prefer to build without Docker, you can do so locally, though this requires manual adjustments.

**Note:** The Docker method is preferred for its consistency, as the local build requires manual path adjustments and assumes the proto/ directory contains the necessary .proto files (atom_space_node.proto and common.proto).

1. Install Poetry: [Follow the instructions at Poetry Installation](https://python-poetry.org/docs/#installation)

2. Execute the commands bellow:

```bash
poetry install

poetry shell

git submodule update --init --recursive

$(pwd)/scripts/build_grpc_files.sh

poetry build --format wheel
```

## Usage

The client can be used in two ways: programmatically as a Python package or via the command line.

### Programmatic Usage

**Note:** For it to work you need a server Query Agent listening on port 35700

After installing the wheel, you can import the pattern_matching_query function from the hyperon_das.main module. Below is an example of querying:

```bash
from hyperon_das.proxies import PatternMatchingQueryProxy
from hyperon_das.service_bus.service_bus import ServiceBusSingleton
from hyperon_das.commons.helpers import tokenize_preserve_quotes, str_2_bool

proxy = PatternMatchingQueryProxy(
    tokens=["LINK_TEMPLATE", "Expression", "3", "NODE", "Symbol", "Similarity", "NODE", "Symbol", '"human"', "VARIABLE", "Y"]
)

service_bus = ServiceBusSingleton(
    host_id='0.0.0.0:9000',
    known_peer='0.0.0.0:40002',
    port_lower=54000,
    port_upper=54500
).get_instance()

service_bus.issue_bus_command(proxy)

while not proxy.finished():
    answer = proxy.pop()
    print(answer.to_string())
```

### Command-Line Usage

You can also run the client directly from the command line after installation. The following command sends the same query as above:

```bash
python -m hyperon_das.main.query_client \
    --client-id localhost:9000 \
    --server-id localhost:40002 \
    --query-tokens "LINK_TEMPLATE Expression 3 NODE Symbol Similarity NODE Symbol "\"human\"" VARIABLE X" \
    --max-query-answers 10
```

The --query-tokens argument accepts a space-separated string, which is tokenized internally while preserving quoted strings (e.g., "Alzheimer's disease").

**Note:** For this to work you need to have the files in .proto/ and generate the grpc files for Python to see **Building locally**
