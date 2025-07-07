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
make run-inference-agent OPTIONS="server_address peer_address <start_port:end_port>"
```
#### Args:
1. **server_address**: The address of the server to connect to, in the form "host:port"
2. **peer_address**: The address of the peer to connect to, in the form "host:port"
3. **\<start_port:end_port\>**: The lower and upper bound for the port numbers to be used by the command proxy

## Run Client
```
make run-inference-agent-client OPTIONS="server_address peer_address <start_port:end_port> REQUEST+"
```
#### Args:
1. **client_address:** The address of the client to connect to, in the form "host:port"
2. **server_address:** The address of the server to connect to, in the form "host:port"
3. **\<start_port:end_port\>:** The lower and upper bound for the port numbers to be used by the command proxy
4. **REQUEST+:** A list of tokens to be sent to the server

## Tests

Run **only** Inference Agent tests
```
 make bazel test //tests/cpp:inference_agent_test
```