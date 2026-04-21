# **Query Agent**

C++ server that performs Pattern Matching queries.

## **Build**

To build the Query Agent, run the following command from the project root:

```bash
make build-all
```

This will generate the binaries for all components in the `das/bin` directory.

## **Prerequisites**

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
  Connected to AttentionBroker at localhost:40001
  SynchronousGRPC listening on localhost:35700
  #############################     REQUEST QUEUE EMPTY     ##################################
  ```
