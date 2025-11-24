# DAS Bus Client and Node Guide

This document provides instructions and examples for running the `bus_client.cc` and `bus_node.cc` programs, adding new proxies and processors, and modifying parameters.


### Compilation
Ensure the project is built using the provided `Makefile`:


```bash
make build-all
```

### Services and Help

Using the `--help` arg displays avaiable services, required args and usage

```bash
make run-busnode OPTIONS="--help"
```

To display help for a specific service run:

```bash
make run-busnode OPTIONS="--service=query-engine --help"
```

## Running a Bus Node


### Execution
Run the `bus_node` binary with the required parameters:

```bash
make run-busnode OPTIONS="--service=<SERVICE_NAME> --endpoint=<ENDPOINT> --ports-range=<PORTS_RANGE>
```

### Examples
#### AtomDB
```
make run-busnode OPTIONS="--service=atomdb-broker --endpoint=localhost:8888 --ports-range=25000:26000"
```
#### Query Engine
```
make run-busnode OPTIONS="--service=query-engine --endpoint=localhost:9000 --ports-range=3000:3100 --attention-broker-endpoint=localhost:37007"
```
#### Evolution Agent
```
make run-busnode OPTIONS="--service=evolution-agent --endpoint=localhost:9001 --ports-range=4000:4100 --attention-broker-endpoint=localhost:37007 --bus-endpoint=localhost:9000"
```
#### LCA
```
make run-busnode OPTIONS="--service=link-creation-agent --endpoint=localhost:9002 --ports-range=4000:4100 --attention-broker-endpoint=localhost:37007 --bus-endpoint=localhost:9000"
```
#### Inference Agent
```
make run-busnode OPTIONS="--service=inference-agent --endpoint=localhost:9003 --ports-range=5000:5100 --attention-broker-endpoint=localhost:37007 --bus-endpoint=localhost:9000"
```


## Running Client


### Execution
Run the `bus_client` binary with the required parameters using make:

```bash
make run-client OPTIONS="--service=<SERVICE_NAME> --endpoint=<ENDPOINT> --bus-endpoint=<BUS_ENDPOINT> --ports-range=<PORTS_RANGE>"
```

### Examples
#### AtomDB:
```
make run-client OPTIONS="--service=atomdb-broker --endpoint=localhost:8887 --bus-endpoint=localhost:8888 --ports-range=27000:28000 --action=add_atoms --tokens=LINK Expression 2 NODE Symbol A NODE Symbol B"
```
#### Query Engine:
```
make run-client OPTIONS="--service=query-engine --endpoint=localhost:8085 --bus-endpoint=localhost:9000 --ports-range=41000:41999 --query=LINK_TEMPLATE Expression 2 NODE Symbol Predicate VARIABLE V1  --context=test --populate-metta-mapping"
```
#### LCA:
```
make run-client OPTIONS="--service=link-creation-agent --endpoint=localhost:8085 --bus-endpoint=localhost:9002 --ports-range=41000:41999 --use-metta-as-query-tokens --max-answers=5 --request='(and (Concept %C1) (Concept %C2)) EQUIVALENCE_RELATION'"
```
or
```
make run-client OPTIONS="--service=link-creation-agent --endpoint=localhost:8085 --bus-endpoint=localhost:9002 --ports-range=41000:41999 --max-answers=5 --request=AND 2 LINK_TEMPLATE Expression 2 NODE Symbol PREDICATE VARIABLE P1 LINK_TEMPLATE Expression 2 NODE Symbol PREDICATE VARIABLE P2 LINK_CREATE Similarity3 2 0 VARIABLE P1 VARIABLE P2"
```
#### Inference Agent
```
 make run-client OPTIONS="--service=inference-agent --endpoint=localhost:8085 --bus-endpoint=localhost:9003 --ports-range=41000:41999 --max-answers=5 --request=PROOF_OF_IMPLICATION 2d01b94c549678aa84840e5901f8d449 ba8267a5c1411eb4bc32d9062e3398ad 2 TOY"
 ```


## Adding a New Element

When adding a new element (proxy or service), follow these steps:

1. **Define the processor type**:
   - Add the new processor type to the `ProcessorType` enum in `Helper.h`.

2. **Define new arguments**:
   - Add any required arguments as static strings in the `Helper` class.

3. **Specify non-optional arguments**:
   - Update the `get_required_arguments` method to include the new arguments for the element.

4. **Update help messages**:
   - Add descriptions for the new element in the `node_service_help` or `client_service_help` maps in `Helper.cc`.


## Adding New Proxies

To add a new proxy, follow the steps in the "Adding a New Element" section, then:

1. Add a case in the `create_proxy` method in `ProxyFactory.h`:

```cpp
case ProcessorType::MY_CUSTOM_PROXY:
    return make_shared<MyCustomProxy>(params);
```

2. Implement the proxy logic in your new class.


## Adding New Services

To add a new service, follow the steps in the "Adding a New Element" section, then:

1. Add a case in the `create_processor` method in `ProcessorFactory.h`:

```cpp
case ProcessorType::MY_CUSTOM_PROCESSOR:
    return make_shared<MyCustomProcessor>(params);
```

2. Implement the processor logic in your new class.


## Modifying Parameters

### Command-Line Parameters
To modify the command-line parameters for `bus_client` or `bus_node`, update the `Helper` class to include new required arguments.

### Proxy Parameters
To modify proxy parameters, use the `set_param` method in `ProxyFactory`:

```cpp
ProxyFactory::set_param(proxy_params, "param_name", "value", ProxyFactory::ParamType::STRING);
```


## Examples of Changes

### Adding a New Bus Element
1. Define the new element in `ProcessorFactory`.
2. Register it in the `bus_node` logic.

### Adding a New Client
1. Define the new client logic in `ProxyFactory`.
2. Register it in the `bus_client` logic.

### Modifying Command-Line Parameters
1. Update the `Helper` class to include new arguments.
2. Parse and validate the arguments in `bus_client` or `bus_node`.

### Modifying Proxy Parameters
1. Update the `ProxyFactory` logic to handle new parameters.
2. Use `set_param` to configure the parameters dynamically.

---

For further details, refer to the source code in `src/main/helpers/` and `src/main/` directories.
