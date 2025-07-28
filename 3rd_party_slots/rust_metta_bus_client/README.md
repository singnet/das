# MeTTa Bus Client (Rust Package)

## Description

This package implements a client to connect and send pattern matching queries to the DAS Service Bus.

## Setup

Clone the repo:
```sh
git clone https://github.com/singnet/das.git
cd das/3rd_party_slots/rust_metta_bus_client
```

Install `rustup`:
```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
# Follow instructions and then:
. "$HOME/.cargo/env"
```

Build the package:
```sh
cargo build --release
```

## Run

If you need to setup a local DAS stack, follow #[] before

After building you can run it by:
```sh
./target/release/metta-bus-client localhost:8080 localhost:35700 0 0 'LINK_TEMPLATE Expression 3 NODE Symbol Similarity NODE Symbol "human" VARIABLE S'
```

You can also use MeTTa S-Expression (unstable):
```sh
./target/release/metta-bus-client localhost:8080 localhost:35700 0 0 'Similarity "human" $S'
```

To control log's level use `RUST_LOG=das=<LEVEL>`:
```sh
RUST_LOG=das=debug ./target/release/metta-bus-client localhost:8080 localhost:35700 0 0 'LINK_TEMPLATE Expression 3 NODE Symbol Similarity NODE Symbol "human" VARIABLE S'
```

## Setup DAS locally

You can use [das-cli](https://github.com/singnet/das-toolbox/tree/master/das-cli) to spawn all necessary services to test DAS locally:
```sh
git clone https://github.com/singnet/das-toolbox
cd das-toolbox

# (Optional) Create a Python Env
python3 -m venv .env
source .env/bin/activate

cd das-cli/src
pip3 install -r requirements.txt

# (Optional) Check installation
python3 das_cli.py --help

# Services Setup
python3 das_cli.py config set

# Start DBs
python3 das_cli.py db start

# Get a MeTTa sample file
curl https://raw.githubusercontent.com/singnet/das-toolbox/refs/heads/master/das-cli/src/examples/data/animals.metta -o /tmp/animals.metta

# Load the MeTTa sample file to DBs
python3 das_cli.py metta load /tmp/animals.metta

# Start Attention Broker
python3 das_cli.py attention-broker start

# Start Query Agent
python3 das_cli.py query-agent start

# (Optional) Check if Docker containers are up and running (redis, mongodb, attention-broker and query-agent)
docker ps -a

# Now you can start sending queries using the './metta-bus-client'
./target/release/metta-bus-client localhost:8080 localhost:35700 0 0 'Similarity "human" $S'
```

