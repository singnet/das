# **Query Agent**  

C++ server that performs Pattern Matching queries.  

## **Build**  

To build the Query Agent, run the following command from the project root:  

```bash
make build-all
```  

This will generate the binaries for all components in the `das/src/bin` directory.  

## **Prerequisites**

Before running the Query Agent, ensure that the following environment variables are properly exported:

- **MongoDB**
  - `DAS_MONGODB_HOSTNAME`
  - `DAS_MONGODB_PORT`
  - `DAS_MONGODB_USERNAME`
  - `DAS_MONGODB_PASSWORD`

- **Redis**
  - `DAS_REDIS_HOSTNAME`
  - `DAS_REDIS_PORT`
  - `DAS_USE_REDIS_CLUSTER` (set to `true` if using Redis Cluster)

- **Attention Broker**
  - `DAS_ATTENTION_BROKER_ADDRESS` (optional, defaults to `localhost` if not provided)
  - `DAS_ATTENTION_BROKER_PORT` (optional, defaults to `37007` if not provided)

You can export these variables by including them in your shell configuration file or by running:

```bash
export DAS_MONGODB_HOSTNAME="localhost"
export DAS_MONGODB_PORT="27017"
export DAS_MONGODB_USERNAME="your-username"
export DAS_MONGODB_PASSWORD="your-password"
export DAS_REDIS_HOSTNAME="localhost"
export DAS_REDIS_PORT="6379"
export DAS_USE_REDIS_CLUSTER=false
export DAS_ATTENTION_BROKER="localhost:37007"
```

Before running the Query Agent, you need to start the Attention Broker. For more information on how to set up and run the Attention Broker, refer to [attention-broker.md](../attention_broker/README.md).


## **Usage**  

You might not be able to execute the binary directly from your machine. To simplify this process, we provide a command to run the service inside a container:  

- Run the Query Agent using the following command:  
  ```bash
  make run-query-agent
  ```  
- The default port for the Attention Broker is `35700`.
- If successful, you should see a message like this:  
  ```bash
  Connected to (NON-CLUSTER) Redis at localhost:6379
  Connected to MongoDB at localhost:27017
  Connected to AttentionBroker at localhost:37007
  SynchronousGRPC listening on localhost:35700
  #############################     REQUEST QUEUE EMPTY     ##################################
  ```  
