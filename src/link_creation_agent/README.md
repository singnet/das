# DAS Link Creation Agent


<!-- ![alt](doc/assets/das_link_creation_diagram.png.png) -->

DAS Link Creation Agent (DAS LCA), process a query and create links using the result of the query and a custom template. The service can execute n times a request to update the database with relevant links over the time.

### Request

The request must have 6 elements:
1. Query (string): A valid Query Agent query (see Query Example)
2. Link Create Template (string): A valid link create template (see Link Create Template Example)
3. Max number of query response (int)
4. Repeat (int): 0 to run once, -1 to run infinitely, 1 or higher to run this number of times.
5. Context (string): Query context for Attention Broker requests 
6. Update Attention Broker flag (bool): true or false to update or not the Attention Broker

Example:
```
LINK_TEMPLATE", "Expression", "3", "NODE", "Symbol", "Similarity","VARIABLE", "V1", "VARIABLE", "V2", "LINK_CREATE", "Similarity", "2", "1", "VARIABLE", "V1", "VARIABLE", "V2", "CUSTOM_FIELD", "truth_value", "2", "CUSTOM_FIELD", "mean", "2", "count", "10", "avg", "0.9", "confidence", "0.9", "10", "0", "test", "false"
```

#### Query Example

```
LINK_TEMPLATE Expression 3 
    NODE Symbol feature_id 
    LINK_TEMPLATE Expression 2 
        NODE Symbol member 
        VARIABLE V1 
        LINK_TEMPLATE Expression 2 
            NODE Symbol feature 
            VARIABLE V2
```

#### Link Create Template Example:

```
LINK_CREATE Similarity 2 1
    VARIABLE V1 
    VARIABLE V2
    CUSTOM_FIELD truth_value 2
        CUSTOM_FIELD mean 2
            count 10
            avg 0.9
        confidence 0.9
```
A link creation request is expected to have 4 elements:
1. LINK_CREATE: required keyword
2. Link type: the type of links that this request will create
3. Link targets count: the count of link targets
4. Custom field count: the count of custom fields

CUSTOM FIELDS

A custom field must have 3 elements:
1. CUSTOM_FIELD: required keyword
2. Custom field name
3. Custom field values count: the count of the values


## How to build

To build the Link Creation Agent, run the following command from the project root:

```bash
make build-all
```

This will generate the binaries for all components in the `das/src/bin` directory.

## How to run

> Before running the Link Creation Agent, you need to start the Query Agent. For more information on how to set up and run the Query Agent, refer to [query-agent.md](../query_engine/README.md).

You might not be able to execute the binary directly from your machine. To simplify this process, we provide a command to run the service inside a container:

```
make run-link-creation-agent OPTIONS="--type server --config_file <path_to_config_file>"
```

The configuration file (`<path_to_config_file>`) must be located in the current directory where the command is executed. This is because we use a volume to mount the file inside the container. If the file is not in the correct location, the command will fail, as the configuration file will not be available inside the container where the binary runs.


#### Config file example

```
requests_interval_seconds = 10
link_creation_agent_thread_count = 5
query_agent_server_id = localhost:35700
query_agent_client_id = localhost:9001
link_creation_agent_server_id = localhost:9080
das_agent_client_id = localhost:9090
das_agent_server_id = localhost:9091
requests_buffer_file = ./buffer
```

* default_interval: The default interval to run requests that repeat one or more times
* thread_count: The number of threads to process Query Agent requests
* query_node_server_id: IP + port of the Query Agent server.
* query_node_client_id: IP + port of the Query Agent client (local machine)
* link_creation_server_id: IP + port of the Link Creation Agent
* das_client_id: IP + port of DAS to add links
* requests_buffer_file: path to request buffer, where are stored all requests that repeat


#### Running client

```
make run-link-creation-client OPTIONS="localhost:1010 localhost:9080 LINK_TEMPLATE Expression 3 NODE Symbol Similarity VARIABLE V1 VARIABLE V2 LINK_CREATE Similarity 2 1 VARIABLE V1 VARIABLE V2 CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9 10 0 test false"
```

If successful, you should see a message like this:

```
Starting server
SynchronousGRPC listening on localhost:9080
SynchronousGRPC listening on localhost:9001
SynchronousGRPC listening on localhost:9090
```
