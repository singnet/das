# **DAS - Attention Broker**

The Attention Broker is the DAS component which keeps track of atom's
importance values attached to different contexts and update those values
according to the queries made by users using context specific hebbian
networks.

DAS query engine can use those importance values to control caching policies
and to better process pattern matcher queries.

## **Build**

To build the Attention Broker Service, run the following command from the project root:

```bash
make build
```

This will generate the binaries for all components in the `das/src/bin` directory.

## **Usage**

- Navigate to the `das/src/bin` directory.
Ensure there is a binary named `attention_broker_service` in this folder.

- Run the service by passing the desired port as an argument:
```bash
./attention_broker_service <PORT>
```

- If successful, you should see a message like this:
```bash
AttentionBroker server listening on localhost:<PORT>
```
