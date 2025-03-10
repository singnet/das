# Non Distributed Query Optimizer

## Usage

Run the optimizer using the main script. The command-line arguments specify the configuration file and the query tokens to optimize.

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
max_generations = 2
max_query_answers = 5
population_size = 500
qtd_selected_for_attention_update = 100
selection_method = "roulette"
fitness_function = "multiply_strengths"
```

#### Running client:

```bash
python das/src/evolution/main.py --config-file /path/to/config.cfg --query-tokens "LINK_TEMPLATE Evaluation 2 NODE Type Name VARIABLE V1"
```

**Parameters:**
--query-tokens: The query string that will be optimized.
--config-file: Path to the configuration file (default is config.cfg).

If successful, you should see a message like this:

```
Starting optimizer
Processing generation 1/n
Results:
['QueryAnswer<1,1> [296bfeeb2ce5148d78f371d0ddf395b2] {(V2: f9a98ccf36f4ba3dcfe2fc99243546fa)} 0.0029751847', 'QueryAnswer<1,1> [2f21f35e3936307c29367adf41aec59e] {(V2: a7d045ace9ea9f9ecbc9094a770cae50)} 0.0025941788', 'QueryAnswer<1,1> [9d6fe9c68e5b29a1b4616ef582e075a3] {(V2: c04cafa6ca7f157321547f4c9ff4bb39)} 0.0025350566', 'QueryAnswer<1,1> [15e8247142c5a46b6079d9df9ea61833] {(V2: f54acd64cd8541c2125588e84da17288)} 0.0024081622', 'QueryAnswer<1,1> [a9335106b2ab5e652749769b72b9e29c] {(V2: 074bd74b5b8c2c87777bf57696ec3edd)} 0.0023660637']
```