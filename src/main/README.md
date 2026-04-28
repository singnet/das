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
make run-busnode OPTIONS="--service=<SERVICE_NAME> --endpoint=<ENDPOINT> --ports-range=<PORTS_RANGE> [--config=<config/das.json>]
```

Optional: pass `--config=<JSON_FILE_PATH>` to load defaults from a JSON config file (schema version 1.0), for example `config/das.json`. Values from the config are used as defaults for the selected service; **command-line arguments always override** config (e.g. `--endpoint=localhost:9002` wins over `agents.query.endpoint` in the file). Node configs include `atomdb`, `loaders`, `agents`, and `brokers`; they do not include `params` (those apply to `bus_client` only).

### Examples

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
#### AtomDB
```
make run-busnode OPTIONS="--service=atomdb-broker --endpoint=localhost:9004 --ports-range=25000:26000 --bus-endpoint=localhost:9000"
```

## Running Client


### Execution
Run the `bus_client` binary with the required parameters using make:

```bash
make run-client OPTIONS="--client=<BUS_NODE_SERVICE> [--endpoint=...] --bus-endpoint=<THIS_CLIENT_HOST:PORT> --ports-range=<PORTS_RANGE> [--config=<config/client.json>]"
```

**`--client`** is required and must be a valid bus node service.

Pass **`--config=<client.json>`** (schema 1.0) so **`atomdb`** can be merged from **`params.das-config-file`** (path to **`das.json`**). Without it, **`bus_client`** cannot initialize AtomDB with the slim client schema. With config, **`--bus-endpoint`**, **`--ports-range`** and **`--endpoint`** may default from **`params`** field and **`das.json`**; optional **`--attention-broker-endpoint`** from merged **`brokers.attention.endpoint`**. Command-line arguments always win.

### Examples
#### AtomDB:
```
make run-client OPTIONS="--client=atomdb-broker --endpoint=localhost:8887 --bus-endpoint=localhost:9000 --ports-range=27000:28000 --action=add_atoms --tokens=LINK Expression 2 NODE Symbol A NODE Symbol B"
```
#### Query Engine:
```
make run-client OPTIONS="--config=config/client.json --client=query-engine --query=LINK_TEMPLATE Expression 2 NODE Symbol Predicate VARIABLE V1 --context=test"
```
(With **`config/client.json`**, **`--endpoint`**, **`--bus-endpoint`**, and **`--ports-range`** can be omitted when **`das.json`** and **`params`** supply them.)

**If you see `Bus: no owner is defined for command <pattern_matching_query>`:** the resolved **`--endpoint`** (from **`agents.<client>.endpoint`** / **`brokers.<client>.endpoint`** in **`das.json`**) must match the **`--endpoint`** used when starting that agent’s **`busnode`** (same `host:port` string).

#### LCA:
```
make run-client OPTIONS="--client=link-creation-agent --endpoint=localhost:8085 --bus-endpoint=localhost:9000 --ports-range=41000:41999 --use-metta-as-query-tokens --max-answers=5 --request='(and (Concept %C1) (Concept %C2)) EQUIVALENCE_RELATION'"
```
or
```
make run-client OPTIONS="--client=link-creation-agent --endpoint=localhost:8085 --bus-endpoint=localhost:9000 --ports-range=41000:41999 --max-answers=5 --request=AND 2 LINK_TEMPLATE Expression 2 NODE Symbol PREDICATE VARIABLE P1 LINK_TEMPLATE Expression 2 NODE Symbol PREDICATE VARIABLE P2 LINK_CREATE Similarity3 2 0 VARIABLE P1 VARIABLE P2"
```
#### Inference Agent
```
 make run-client OPTIONS="--client=inference-agent --endpoint=localhost:8085 --bus-endpoint=localhost:9000 --ports-range=41000:41999 --max-answers=5 --request=PROOF_OF_IMPLICATION 2d01b94c549678aa84840e5901f8d449 ba8267a5c1411eb4bc32d9062e3398ad 2 TOY"
 ```

#### Context Broker
```
 make run-client OPTIONS="--client=context-broker --endpoint=localhost:8086 --bus-endpoint=localhost:9000 --ports-range=41000:41999  --query=OR 3 AND 4 LINK_TEMPLATE Expression 3 NODE Symbol Evaluation VARIABLE V0 LINK_TEMPLATE Expression 2 NODE Symbol Concept VARIABLE S1 LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE S1 VARIABLE W1 LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE S2 VARIABLE W1 LINK_TEMPLATE Expression 3 NODE Symbol Evaluation VARIABLE V1 LINK_TEMPLATE Expression 2 NODE Symbol Concept VARIABLE S2 LINK_TEMPLATE Expression 3 NODE Symbol Implication VARIABLE V0 VARIABLE V1 LINK_TEMPLATE Expression 3 NODE Symbol Implication VARIABLE V1 VARIABLE V0 --determiner-schema=V0:V1,V0:S2 --stimulus-schema=V0"
 ```

#### Evolution Agent

```
make run-client OPTIONS="--client=evolution-agent --endpoint=localhost:8085 --bus-endpoint=localhost:9000 --ports-range=41000:41999  --query='(Contains %sentence1 (Word \"bbb\"))' --correlation-queries='(Contains %placeholder1 %word1)' --correlation-replacements=%placeholder1:sentence1 --correlation-mappings=sentence1:%word1 --max-generations=3 --fitness-function-tag=count_letter --context=Aaa --use-metta-as-query-tokens --max-answers=50"
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
