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
PROOF_OF_IMPLICATION_OR_EQUIVALENCE <handle1> <handle2> <max proof length>
PROOF_OF_IMPLICATION <handle1> <handle2> <max proof length>
PROOF_OF_EQUIVALENCE <handle1> <handle2> <max proof length>
```

### Example

```
PROOF_OF_IMPLICATION_OR_EQUIVALENCE "handle1" "handle2" "10"
```

## Build

```
make build
```

## Run

TBD

