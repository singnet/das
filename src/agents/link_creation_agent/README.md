# DAS Link Creation Agent


<!-- ![alt](doc/assets/das_link_creation_diagram.png.png) -->

DAS Link Creation Agent (DAS LCA), process a query and create links using the result of the query and a custom template. The service can execute n times a request to update the database with relevant links over the time.

### Request

The request must have 8 elements:
1. MAX_ANSWERS (int): Maximum number of anwers to be processed
2. REPEAT_COUNT (int): Number of times the request will query and create links (set to 0 to infinite)
3. ATTENTION_UPDATE_FLAG (bool): Whether to update the attention broker (true/false)
4. POSITIVE_IMPORTANCE_FLAG (bool): Query Agent should return only links with importance greater than zero (true/false)
5. USE_METTA_AS_QUERY_TOKENS (bool): Use MeTTa expressions as query and template to create links
6. CONTEXT (string): Query context for Attention Broker requests
7. Query (string): A valid Query Agent query (see Query Example) or MeTTa expression if USE_METTA_AS_QUERY_TOKENS is set as true
8. Link Create Template (string): A valid link create template (see Link Create Template Example) or MeTTa expression if USE_METTA_AS_QUERY_TOKENS is set as true or a processor

Processors:
```
PROOF_OF_IMPLICATION
PROOF_OF_EQUIVALENCE
```


Example (Tokens):
```
AND 2 LINK_TEMPLATE Expression 2 NODE Symbol PREDICATE VARIABLE P1 LINK_TEMPLATE Expression 2 NODE Symbol PREDICATE VARIABLE P2 LINK_CREATE Similarity 2 1 VARIABLE P1 VARIABLE P2 CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9
```

Example (MeTTa):
```
'(and (PREDICATE %P1) (PREDICATE %P2))' '(IMPLICATION %P1 %P2)'
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

This will generate the binaries for all components in the `bin` directory.

## How to run

> Before running the Link Creation Agent, you need to start the Query Agent. For more information on how to set up and run the Query Agent, refer to [query-agent.md](../query_engine/README.md).

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

Example:
```
 make run-link-creation-client OPTIONS="<client_address> <server_address> <start_port:end_port> <max_answers> <repeat_count> <attention_update_flag> <positive_importance_flag> <use_metta_as_query_tokens> <context> <query_tokens or metta expression> <linkcreate_tokens or metta expression>"
```

Tokens:
```
 make run-link-creation-client OPTIONS="localhost:9085 localhost:9080 1777:1888 50 1 false false false context1 AND 2 LINK_TEMPLATE Expression 2 NODE Symbol PREDICATE VARIABLE P1 LINK_TEMPLATE Expression 2 NODE Symbol PREDICATE VARIABLE P2 LINK_CREATE Similarity 2 1 VARIABLE P1 VARIABLE P2"
```
MeTTa:
```
make run-link-creation-client OPTIONS="localhost:9085 localhost:9080 1777:1888 50 1 false false true context1 '(and (PREDICATE %P1) (PREDICATE %P2))' '(IMPLICATION %P1 %P2)'"
```
##### OPTIONS parameters are:
* client_address: The address of the client (localhost:1010)
* server_address: The address of the server (localhost:9080)
* \<start_port:end_port\> The lower and upper bound for the port numbers to be used by the command proxy
* max_answers: ax number of responses of the query to process
* repeat_count: Repeat the query and link creations, should be greater than 0, 0 is to run the process for ever
* attention_update_flag: "true" if this query should update the Attention Broker (false)
* positive_importance_flag: "true" if this query should only return answers thar have a importance value greater than zero
* use_metta_as_query_tokens: "true" if this request is using MeTTa expressions as query and creation template
* context: Context of the query (test)
* query: Base token or MeTTa query to create links 
* create_template: Template or MeTTa to create the new links

If successful, you should see a message like this:

```
2025-10-18 10:48:16 | [INFO] | SynchronousGRPC listening on localhost:9080
2025-10-18 10:48:17 | [INFO] | LinkCreationAgent initialized with request interval: ... seconds
2025-10-18 10:48:17 | [INFO] | LinkCreationAgent initialized with Query timeout: .. seconds
```
