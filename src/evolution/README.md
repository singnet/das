# Non Distributed Query Optimizer

## How to build

To build run the following command from the project root:

```bash
make build-all
```

This will generate the binary file in the `das/src/bin` directory.

## Usage

Run the optimizer using the main script. The command-line arguments specify the configuration file.

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
query_agent_node_id = "localhost:31701"
query_agent_server_id = "localhost:31700"
attention_broker_server_id = "localhost:37007"
evolution_server_id = "localhost:5020"
max_generations = 2
max_query_answers = 5
population_size = 500
qtd_selected_for_attention_update = 100
selection_method = "roulette"
fitness_function = "multiply_strengths"
```

#### Running:

```bash
make run-evolution OPTIONS='--config-file /path/to/config.cfg'
```

**Parameters:**
--config-file: Path to the configuration file (default is config.cfg).

If successful, you should see a message like this:

```bash
SynchronousGRPC listening on localhost:5020
Running server...
```