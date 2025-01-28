# **Query Agent**  

C++ server that performs Pattern Matching queries.  

## **Build**  

To build the Query Agent, run the following command from the project root:  

```bash
make build
```  

This will generate the binaries for all components in the `das/src/bin` directory.  

## **Usage**  

You might not be able to execute the binary directly from your machine. To simplify this process, we provide a command to run the service inside a container:  

- Run the Query Agent using the following command:  
  ```bash
  PORT=1000 make run-query-agent
  ```  

- Replace `1000` with the desired port number.  

- If successful, you should see a message like this:  
  ```bash
  Query Agent server listening on localhost:<PORT>
  ```  
