# OpenCog Hyperon - Distributed Atomspace (DAS)

**_IMPORTANT NOTE:_** _we're currently re-structuring the project repo, documentation etc. So directory structure, documentation files, etc may be outdated or somehow messy._

## **Components**

DAS consists of several components. Below is a list of these components, categorized by whether they are part of this repository or exist in external repositories.

### **Internal Components**
The following components are included in this repository:  

- **Attention Broker**
  A DAS component that tracks atom importance values in different contexts and updates those values based on user queries using context-specific Hebbian networks.  

  Documentation: [Read more](src/cpp/attention_broker/README.md)

- **Query Agent**

  C++ server which perform Pattern Matching queries.

  Documentation: [Read more](src/cpp/query_engine/README.md)

- **Link Creation Agent**
  A C++ server that performs pattern-matching queries and creates new links in the Atomspace based on the results.  

  Documentation: [Read more](src/cpp/link_creation_agent/README.md)

- **Inference Control**
  C++ server which perform Inference Queries.

  Link creation engine searches the atom space for patterns and creates new links based on the results. Both the pattern and the layout of the new links to be created are specified by the caller.

  Pattern matching results are not exhaustively used to create new links. DAS query engine return results considering importance values of atoms in each result so link creation engine iterates only through the first N results, which have a greater probability of having the most interesting atoms. 

  Documentation: *Coming Soon*  

- **Query Engine**
  A data manipulation API for Distributed Atomspace (DAS). It allows queries with pattern matching capabilities and traversal of the Atomspace hypergraph.  

  Documentation: [Read more](src/python/hyperon_das/README.md)

- **Atom DB**
  A Python library required by Hyperon DAS to interface with DBMSs and other data storage structures.  

  Documentation: [Read more](src/python/hyperon_das_atomdb/README.md)  

---

### **External Components**
The following components are part of external repositories and are not included in this repository. You can find them in their respective locations:  

- **DAS Server**

  A server listening on a port. Essentially, this is what we currently call DAS Server which is OpenFaas running das-query-engine function (basically our DistributedAtomSpace class python class).

  Documentation: [Read more](https://github.com/singnet/das-serverless-functions)

- **DAS MeTTa Parser**
  A simplified MeTTa parser used to feed knowledge bases into DAS.  

  Documentation: [Read more](https://github.com/singnet/das-metta-parser)

- **DAS Server Toolbox**

  This CLI provides a set of commands to manage containerized services, OpenFaaS functions, and Metta operations. It includes functionalities to start and stop db services, manage OpenFaaS functions, load Metta files, and validate Metta file syntax.

  Documentation: [Read more](https://github.com/singnet/das-toolbox)

- **Infrastructure Setup**

  Tools for provisioning and setting up servers on cloud providers.  

  Documentation:  
  - [Server provisioning in Vultr](infrastructure/vultr/provisioning/README.md)  
  - [Server setup Vultr](infrastructure/vultr/setup/README.md)  
  - [Server provisioning in AWS](infrastructure/aws/provisioning/README.md)  
  - [Server setup AWS](infrastructure/aws/setup/README.md)

## **Getting Started**

### **1. Cloning the Repository**
```bash
git clone git@github.com:singnet/das.git
cd das
```

### **2. Build**

To build the agents, run the command:

```bash
make build
```

This process will generate the binaries for all components in the `das/src/bin` directory.

### **3. Initializing the Knowledge Base**

Some of the components require an existing knowledge base and a properly configured environment with Redis and MongoDB. You can set up this environment using the `das-cli`. For detailed instructions, refer to the [das-toolbox documentation](https://github.com/singnet/das-toolbox).

> For detailed information about each component, please refer to the documentation provided for each component in the [**Components**](#components) section above. 

## **Running the Unit Tests**  

To execute the unit tests, simply run the following command from the project root:  

```bash
make tests
```  

This will run all the tests for the project.  

## **Conceptual and User's Documentation**

* [DAS API](https://singnet.github.io/das-query-engine/api/DAS/) - Documentation for the DAS API.
* DAS User's Guide ([Jupyter Notebook](notebooks/das-users-guide.ipynb)) ([Markdown](docs/das-users-guide.md)) - Document with code snippets showing how to use DAS.
* [DAS Overview](docs/das-overview.md) - Description of the main DAS components and deployment architecture.
* [DAS Toolbox User's Guide](https://github.com/singnet/das-toolbox) - A guide with detailed instructions to use the DAS Server Toolbox.
* [Nunet Deployment Guide](docs/nunet-deployment-guide.md) - A comprehensive guide for deploying DAS components to the NuNet network.
* [Release Notes](docs/release-notes.md) - Release notes for all DAS components.

## **Project Management**

* [Public board](https://github.com/orgs/singnet/projects/7) - GitHub project board used to track bug reports, feature requests, and major new features planning. Use this board to report bugs or request new features.
* [Development board](https://github.com/orgs/singnet/projects/6/views/1) - Used internally by the DAS Team to track issues and tasks.
* _Contribution guidelines_
