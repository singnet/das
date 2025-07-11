# DAS LCALink Creation Agent


<!-- ![alt](doc/assets/das_link_creation_diagram.png.png) -->

DAS LCALink Creation Agent (DAS LCA), process a query and create links using the result of the query and a custom template. The service can execute n times a request to update the database with relevant links over the time.

### Request

The request must have 6 elements:
1. Query (string): A valid Query Agent query (see Query Example)
2. LCALink Create Template (string): A valid link create template (see LCALink Create Template Example)
3. Max number of query response (int)
4. Repeat (int): 0 to run once, -1 to run infinitely, 1 or higher to run this number of times.
5. Context (string): Query context for Attention Broker requests
6. Update Attention Broker flag (bool): true or false to update or not the Attention Broker
7. Optional Request ID

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

#### LCALink Create Template Example:

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
2. LCALink type: the type of links that this request will create
3. LCALink targets count: the count of link targets
4. Custom field count: the count of custom fields

CUSTOM FIELDS

A custom field must have 3 elements:
1. CUSTOM_FIELD: required keyword
2. Custom field name
3. Custom field values count: the count of the values


## How to build

To build the LCALink Creation Agent, run the following command from the project root:

```bash
make build-all
```

This will generate the binaries for all components in the `das/bin` directory.

## How to run

> Before running the LCALink Creation Agent, you need to start the Query Agent. For more information on how to set up and run the Query Agent, refer to [query-agent.md](../query_engine/README.md).

You might not be able to execute the binary directly from your machine. To simplify this process, we provide a command to run the service inside a container, the parameters inside the square brackets are optional:

```
make run-link-creation-agent OPTIONS="link_creation_agent server_address peer_address <start_port:end_port> [request_interval] [thread_count] [default_timeout] [buffer_file] [metta_file_path] [save_links_to_metta_file] [save_links_to_db]"
```



#### Parameters and default values

* server_address: The address of the server to connect to, in the form "host:port"
* peer_address: The address of the peer to connect to, in the form "host:port"
* \<start_port:end_port\> The lower and upper bound for the port numbers to be used by the command proxy
* request_interval: The interval in seconds to send requests (default: 1)
* thread_count: The number of threads to process requests (default: 1)
* default_timeout: The timeout in seconds for query requests (default: 10)
* buffer_file: The path to the requests buffer file (default: "requests_buffer.bin")
* metta_file_path: The path to the metta file (default: "./")
* save_links_to_metta_file: Whether to save links to the metta file (default: true)
* save_links_to_db: Whether to save links to the database (default: false)



#### Running client

```
make run-link-creation-client OPTIONS="localhost:1010 localhost:9080 <start_port:end_port> LINK_TEMPLATE Expression 3 NODE Symbol Similarity VARIABLE V1 VARIABLE V2 LINK_CREATE Similarity 2 1 VARIABLE V1 VARIABLE V2 CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9 10 1 test false"
```
##### OPTIONS parameters are:
* client_host: The address of the client (localhost:1010)
* server_host: The address of the server (localhost:9080)
* \<start_port:end_port\> The lower and upper bound for the port numbers to be used by the command proxy
* query: Base query to create links (LINK_TEMPLATE Expression 3 NODE Symbol Similarity VARIABLE V1 VARIABLE V2)
* create_template: Template to create the new links (LINK_CREATE Similarity 2 1 VARIABLE V1 VARIABLE V2 CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9)
* max_result: Max number of responses of the query to process (10)
* repeat: Repeat the query and link creations, should be greater than 0, 0 is reserved for other agents (1)
* context: Context of the query (test)
* update_attention_broker: "true" if this query should update the Attention Broker (false)


If successful, you should see a message like this:

```
Starting server
SynchronousGRPC listening on localhost:9080
SynchronousGRPC listening on localhost:9001
SynchronousGRPC listening on localhost:9090
```
