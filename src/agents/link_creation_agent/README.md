# Link Creation Agent (LCA)

The **Link Creation Agent (LCA)** is a service that automatically generates new expressions (links) within a knowledge base.  
It processes queries and creates links using the results of those queries in combination with a **custom template** or a specialized **processor**.  
An **LCA Request** can be executed a specific number of times or configured to run indefinitely, continuously updating the database with relevant links over time.

---

## LCA Request

An **LCA Request** defines how the agent performs queries and generates links.  
Each **request cycle** consists of the following steps:
1. The agent performs a pattern-matching query according to the specified parameters.  
2. The query results are collected and processed.  
3. New expressions (links) are generated using the defined template or processor.  
4. The generated links are inserted into the knowledge base before the next cycle begins.  

This cyclic process enables the system to evolve dynamically, discovering and reinforcing semantic relationships over time.

---

### Request Structure

A request must contain **8 elements**:

1. **MAX_ANSWERS (int)** — Maximum number of query answers to process.  
2. **REPEAT_COUNT (int)** — Number of times the request will perform the query and create links. Set to `0` to run indefinitely.  
3. **ATTENTION_UPDATE_FLAG (bool)** — Whether to update importance values in the Attention Broker.  
4. **POSITIVE_IMPORTANCE_FLAG (bool)** — If `true`, only answers with importance greater than zero are returned.  
5. **USE_METTA_AS_QUERY_TOKENS (bool)** — If `true`, MeTTa expressions are used as the query and creation template.  
6. **CONTEXT (string)** — The query context used by the Attention Broker.  
7. **Query (string)** — A valid Query Agent query (see example) or a MeTTa expression if `USE_METTA_AS_QUERY_TOKENS` is enabled.  
8. **Link Create Template (string)** — A valid link creation template or processor expression.  

**Available processors:**
```
IMPLICATION_RELATION
EQUIVALENCE_RELATION
```

---

### Example (Tokens)

```
AND 2 LINK_TEMPLATE Expression 2 NODE Symbol Predicate VARIABLE P1 LINK_TEMPLATE Expression 2 NODE Symbol Predicate VARIABLE P2 LINK_CREATE CustomRelation 2 1 VARIABLE P1 VARIABLE P2 CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9
```

### Example (MeTTa)

```
'(and (Predicate %P1) (Predicate %P2))' '(CustomRelation %P1 %P2)'
```

---

## Query Example

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

---

## Link Create Template Example

```
LINK_CREATE CustomRelation 2 1
    VARIABLE V1
    VARIABLE V2
    CUSTOM_FIELD truth_value 2
        CUSTOM_FIELD mean 2
            count 10
            avg 0.9
        confidence 0.9
```

A link creation request is expected to have 4 elements:
1. `LINK_CREATE`: Required keyword.  
2. **Link type**: The type of link to be created.  
3. **Link targets count**: The number of target nodes for the link.  
4. **Custom field count**: The number of custom fields.  

### Custom Fields

A custom field must have 3 elements:
1. `CUSTOM_FIELD`: Required keyword.  
2. **Field name**: The name of the custom field.  
3. **Values count**: The number of values defined for that field.  

---

## How to Build

To build the Link Creation Agent, execute the following command from the project root:

```bash
make build-all
```

This command compiles all components and places the binaries in the `bin` directory.

---

## How to Run

> Before running the Link Creation Agent, ensure that the **Query Agent** is running.  
> For setup and usage details, refer to [query-agent.md](../query_engine/README.md).

To simplify execution, the agent can be run inside a container.  
The parameters in square brackets are optional:

```bash
make run-link-creation-agent OPTIONS="link_creation_agent server_address peer_address <start_port:end_port> [request_interval] [thread_count] [default_timeout] [buffer_file] [metta_file_path] [save_links_to_metta_file] [save_links_to_db]"
```

---

### Parameters and Default Values

| Parameter | Description | Default |
|------------|-------------|----------|
| **server_address** | Address of the server in `host:port` format. | — |
| **peer_address** | Address of the peer in `host:port` format. | — |
| **<start_port:end_port>** | Port range used by the command proxy. | — |
| **request_interval** | Interval (seconds) between request cycles. | `1` |
| **thread_count** | Number of worker threads used for processing. | `1` |
| **default_timeout** | Timeout (seconds) for each query request. | `10` |
| **buffer_file** | Path to the request buffer file. | `requests_buffer.bin` |
| **metta_file_path** | Path to the MeTTa file directory. | `./` |
| **save_links_to_metta_file** | Whether to save links to a MeTTa file. | `true` |
| **save_links_to_db** | Whether to save links to the database. | `false` |

---

## Running the Client

### Example (Tokens)

```bash
make run-link-creation-client OPTIONS="localhost:9085 localhost:9080 1777:1888 50 1 false false false context1 AND 2 LINK_TEMPLATE Expression 2 NODE Symbol Predicate VARIABLE P1 LINK_TEMPLATE Expression 2 NODE Symbol Predicate VARIABLE P2 LINK_CREATE CustomRelation 2 1 VARIABLE P1 VARIABLE P2"
```

### Example (MeTTa)

```bash
make run-link-creation-client OPTIONS="localhost:9085 localhost:9080 1777:1888 50 1 false false true context1 '(and (Predicate %P1) (Predicate %P2))' '(CustomRelation %P1 %P2)'"
```

### Client Parameters

| Parameter | Description |
|------------|-------------|
| **client_address** | Address of the client (`localhost:1010`). |
| **server_address** | Address of the LCA server (`localhost:9080`). |
| **<start_port:end_port>** | Range of ports used by the command proxy. |
| **max_answers** | Maximum number of query results to process. |
| **repeat_count** | Number of query/link creation cycles. Set to `0` to run indefinitely. |
| **attention_update_flag** | Whether to update the Attention Broker. |
| **positive_importance_flag** | If `true`, only results with positive importance are returned. |
| **use_metta_as_query_tokens** | If `true`, use MeTTa expressions for query and template. |
| **context** | Context of the query. |
| **query** | Base token or MeTTa query used to create links. |
| **create_template** | Template or MeTTa expression defining link creation. |

---

### Example Output

If successful, you should see messages similar to the following:

```
2025-10-18 10:48:16 | [INFO] | SynchronousGRPC listening on localhost:9080
2025-10-18 10:48:17 | [INFO] | LinkCreationAgent initialized with request interval: ... seconds
2025-10-18 10:48:17 | [INFO] | LinkCreationAgent initialized with query timeout: ... seconds
```
