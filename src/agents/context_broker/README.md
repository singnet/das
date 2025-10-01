# **Context Broker**

C++ server that performs Attention Broker context creation.

## **Build**

To build the Context Broker, run the following command from the project root:

```bash
make build-all
```

This will generate the binaries for all components in the `das/bin` directory.

## **Prerequisites**

Before running the Context Broker, ensure that the following environment variables are properly exported (or set in `.env` file):

- **MongoDB**
  - `DAS_MONGODB_HOSTNAME`
  - `DAS_MONGODB_PORT`
  - `DAS_MONGODB_USERNAME`
  - `DAS_MONGODB_PASSWORD`

- **Redis**
  - `DAS_REDIS_HOSTNAME`
  - `DAS_REDIS_PORT`
  - `DAS_USE_REDIS_CLUSTER` (set to `true` if using Redis Cluster)

You can export these variables by including them in your shell configuration file or by running:

```bash
export DAS_MONGODB_HOSTNAME="localhost"
export DAS_MONGODB_PORT="27017"
export DAS_MONGODB_USERNAME="your-username"
export DAS_MONGODB_PASSWORD="your-password"
export DAS_REDIS_HOSTNAME="localhost"
export DAS_REDIS_PORT="6379"
export DAS_USE_REDIS_CLUSTER=false
```

Before running the Context Broker, you need to start the Attention Broker. For more information on how to set up and run the Attention Broker, refer to [attention-broker](../../attention_broker/README.md).


## **Usage**

You might not be able to execute the binary directly from your machine. To simplify this process, we provide a command to run the service inside a container:

- Run the Context Broker using the following command:
```bash
# Note: OPTIONS="<hostname:port> <start_port:end_port> <peer_ip:peer_port> <AB_ip:AB_port>"
make run-context-broker OPTIONS="0.0.0.0:38000 42000:42999 0.0.0.0:35700 0.0.0.0:40001"
```
- The default port for the Attention Broker is `40001`.
- If successful, you should see a message like this:
```bash
2025-10-01 17:51:00 | [INFO] | Starting context broker server with id: 0.0.0.0:38000
2025-10-01 17:51:00 | [INFO] | Using default MongoDB chunk size: 1000
2025-10-01 17:51:00 | [INFO] | Connected to MongoDB at 0.0.0.0:28000
2025-10-01 17:51:00 | [INFO] | Connected to (NON-CLUSTER) Redis at 0.0.0.0:29000
2025-10-01 17:51:00 | [INFO] | BUS static initialization
2025-10-01 17:51:00 | [INFO] | BUS command: <context>
2025-10-01 17:51:00 | [INFO] | BUS command: <inference>
2025-10-01 17:51:00 | [INFO] | BUS command: <link_creation>
2025-10-01 17:51:00 | [INFO] | BUS command: <pattern_matching_query>
2025-10-01 17:51:00 | [INFO] | BUS command: <query_evolution>
2025-10-01 17:51:00 | [INFO] | Port range: [42000 : 42999]
2025-10-01 17:51:00 | [INFO] | SynchronousGRPC listening on 0.0.0.0:38000
2025-10-01 17:51:01 | [INFO] | Calling AttentionBroker GRPC. Ping
2025-10-01 17:51:01 | [INFO] | Connected to AttentionBroker at 0.0.0.0:40001
2025-10-01 17:51:01 | [INFO] | BUS node 0.0.0.0:38000 is taking ownership of command context
2025-10-01 17:51:01 | [INFO] | #############################     REQUEST QUEUE EMPTY     ##################################
```
