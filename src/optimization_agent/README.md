# Distributed Query Optimizer

A Python-based evolutionary algorithm designed to optimize query answers using distributed computing. This project avoids scanning all possible answers by leveraging a user-defined fitness function to rank and boost promising query components (atoms) through an Attention Broker. Multiple servers work together via GRPC to generate diverse candidate populations and refine the best answers.

## Table of Contents

- [Distributed Query Optimizer](#distributed-query-optimizer)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Features](#features)
  - [Architecture](#architecture)
    - [Leader Node](#leader-node)
    - [Worker Node](#worker-node)
  - [Usage](#usage)
      - [Config file example](#config-file-example)
      - [Running client:](#running-client)

## Overview

The Distributed Query Optimizer uses an evolutionary algorithm to find the best query answers. It repeatedly:
- Samples a population of answers.
- Evaluates them using a fitness function.
- Selects top candidates to update the sorting mechanism via an Attention Broker.

The optimizer is built **Distributed Algorithm Node**, where:
- A **Leader Node** coordinates the optimization process.
- Multiple **Worker Nodes** perform sampling, evaluation, and reporting using GRPC-based messaging.

## Features

- **Evolutionary Algorithm:**  
  - Samples candidate query answers.
  - Evaluates each candidate using a customizable fitness function.
  - Selects the best candidates using defined selection methods.

- **Attention Broker Integration:**  
  - Aggregates promising atom handles.
  - Boosts query optimization by stimulating the Attention Broker with aggregated data.

- **Distributed Architecture:**  
  - Leader-Worker model using GRPC for communication.

## Architecture

### Leader Node
- **Role:**  
  Coordinates generations, aggregates results from workers, and updates the Attention Broker.
- **Key Responsibilities:**  
  - Broadcasting generation start messages.
  - Waiting for worker responses.
  - Selecting best individuals and updating the Attention Broker.
  - Managing worker status and lock requests.

### Worker Node
- **Role:**  
  Samples query answers, evaluates their fitness, and reports results back to the Leader Node.
- **Key Responsibilities:**  
  - Sampling a population using a remote iterator.
  - Evaluating each answer with a configured fitness function.
  - Handling lock requests to synchronize critical operations.

## Usage

Run the optimizer using the main script. The command-line arguments specify the number of servers (nodes) and the query tokens to optimize.

#### Config file example

```
mongo_hostname = "localhost"
mongo_port = 27017
mongo_username = "root"
mongo_password = "root"
redis_hostname = "localhost"
redis_port = 6379
redis_cluster = False
redis_ssl = False
max_generations = 3
max_query_answers = 5
qtd_selected_for_attention_update = 100
selection_method = "roulette"
population_size = 500
fitness_function = "multiply_strengths"
```

#### Running client:

```bash
python das/src/optimization_agent/main.py --number-servers 4 --query-tokens "sample query tokens" --config-file /path/to/config.cfg
```

**Parameters:**
--number-servers: Total number of servers in the network. The first server acts as the Leader, and the rest are Worker Nodes.
--query-tokens: The query string that will be optimized.
--config-file: Path to the configuration file (default is config.cfg).

If successful, you should see a message like this:

```
Starting optimizer
[Leader.start_generation] - generation: 1
[Leader.start_generation] - generation: 2
[Leader.start_generation] - generation: 3
['QueryAnswer<1,1> [438c13716c53d8e706961d3995140c87] {(V2: 1413b251af430a07ac0810e18acc78b8)} 0.002139624', 'QueryAnswer<1,1> [3c83be1cf6f8143f5d158205cb61766b] {(V2: a13c2f82bd7809c7ae649e1806824507)} 0.0020572757', 'QueryAnswer<1,1> [596a89ed04400c0ca8a69096c8ef29ce] {(V2: 502d40809c3252afd7d6b2e6d7bab4db)} 0.0020572757', 'QueryAnswer<1,1> [ff18240f01721e6d5100e937d4245052] {(V2: 19669ffef6f6d72400fba2a335438b76)} 0.0020542662', 'QueryAnswer<1,1> [0d120641b703288828e0b802ac1473aa] {(V2: 83e0b07b3e8b8bcddcbd4bc8b4ef49c4)} 0.0020129767']
```