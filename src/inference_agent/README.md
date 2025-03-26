# DAS Inference Agent

The DAS Inference Agent is a component that orchestrates the distributed inference process. It receives inference requests from the client, sends link creation requests to the link creation agent, and listens for the distributed inference to finish stopping the link creation process.

## Request

Inference Agent processes inference requests. There will be several different types of inferences requests the following are the types of requests that can be processed:

    PROOF_OF_IMPLICATION_OR_EQUIVALENCE
    PROOF_OF_IMPLICATION
    PROOF_OF_EQUIVALENCE

Each request consists of a command and a list of parameters. The first parameter is the processor id, the second and third parameters are the handles of the two nodes to be connected and the last parameter is the maximum proof length.

The following are the commands and their corresponding parameters:

```
PROOF_OF_IMPLICATION_OR_EQUIVALENCE <handle1> <handle2> <max proof length> <context>
PROOF_OF_IMPLICATION <handle1> <handle2> <max proof length> <context>
PROOF_OF_EQUIVALENCE <handle1> <handle2> <max proof length> <context>
```

### Example

```
PROOF_OF_IMPLICATION_OR_EQUIVALENCE "handle1" "handle2" "10" "inference_context"
```

## Build

```
make build-all
```

## Run Server

```
make run-inference-agent OPTIONS="path_to_config_file"
```

## Run Client
```
make run-inference-agent-client OPTIONS="CLIENT_HOST:CLIENT_PORT SERVER_HOST:SERVER_PORT REQUEST+"
```


## Tests

Run **only** Inference Agent tests
```
 make bazel test //tests/cpp:inference_agent_test
```


## Config

The config file is a text file that contains the following:

```
inference_node_id = <inference_node_id>
link_creation_agent_client_id = <link_creation_agent_client_id>
link_creation_agent_server_id = <link_creation_agent_server_id>
das_client_id = <das_client_id>
das_server_id = <das_server_id>
distributed_inference_control_node_id = <distributed_inference_control_node_id>
distributed_inference_control_node_server_id = <distributed_inference_control_node_server_id>
```

Example:

```
inference_node_id = "localhost:8080"
link_creation_agent_client_id = "localhost:8081"
link_creation_agent_server_id = "localhost:8082"
das_client_id = "localhost:8083"
das_server_id = "localhost:8084"
distributed_inference_control_node_id = "localhost:8085"
distributed_inference_control_node_server_id = "localhost:8086"
```


