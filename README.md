# OpenCog Hyperon - Distributed Atomspace (DAS)

**_ATTENTION:_** _we're currently re-structuring the project repo, documentation etc. So directory structure, documentation files, etc may be outdated or somehow messy._

## **Components**

DAS consists of several components. Below is a list of these components:

- **Query Agent**

  A data manipulation API for Distributed Atomspace (DAS). It allows queries with pattern matching capabilities and traversal of the Atomspace hypergraph.  

  Documentation: [Read more](docs/components/query-agent.md)

- **Attention Broker**

  DAS component that keeps track of atom's importance values attached to different contexts and updates those values according to the queries made by users using context-specific Hebbian networks.  

  Documentation: [Read more](docs/components/attention-broker.md)

- **Link Creation Agent**

  C++ server which perform Pattern Matching queries.

  Link creation engine searches the atom space for patterns and creates new links based on the results. Both the pattern and the layout of the new links to be created are specified by the caller.

  Pattern matching results are not exhaustively used to create new links. DAS query engine return results considering importance values of atoms in each result so link creation engine iterates only through the first N results, which have a greater probability of having the most interesting atoms.


- **Database Wrapper**

  SQL Tables are mapped by an unsupervised algorithm which uses the database scheme obtained from the DBMS to extract table/field names, primary/foreign key definitions etc to create a structure of nodes and links which represents the same information. No assumptions are made on database structure and no preprocessing is performed.

  Basically, all table contents are mapped to basic nodes/links used in logic inference such as InheritanceLink, SchemeNode, ConceptNode, ExecutionLink, etc.

  Documentation: [Read more](docs/components/database-wrapper.md)

- **Inference Control**

  C++ server which perform Inference Queries.

  Link creation engine searches the atom space for patterns and creates new links based on the results. Both the pattern and the layout of the new links to be created are specified by the caller.

  Pattern matching results are not exhaustively used to create new links. DAS query engine return results considering importance values of atoms in each result so link creation engine iterates only through the first N results, which have a greater probability of having the most interesting atoms.


- **Atom DB**

  Source code for hyperon-das-atomdb, a Python library required by hyperon-das to interface with DBMSs and other data storage structures.  

  Documentation: [Read more](docs/components/atomdb.md)

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

  Tools and assets required to provision and set up servers in cloud providers.  

  Documentation:  
  - [Server provisioning in Vultr](infrastructure/vultr/provisioning/README.md)  
  - [Server setup Vultr](infrastructure/vultr/setup/README.md)  
  - [Server provisioning in AWS](infrastructure/aws/provisioning/README.md)  
  - [Server setup AWS](infrastructure/aws/setup/README.md)


## **Getting Started**

### **1. Cloning the Repository**
```bash
git clone <repo-url>
cd das
```

### **2. Build**

To build the agents, run the command:

```bash
make build
```

This process will generate the binaries for all components in the `das/src/bin` directory. For detailed information about each component, please refer to the documentation provided for each component in the [**Components**](#components) section above. 

## **Running the Tests**  

To execute the tests, simply run the following command from the project root:  

```bash
make tests
```  

This will run all the tests for the project.  

## **Documentation**

* [DAS API](https://singnet.github.io/das-query-engine/api/DAS/) - Documentation for the DAS API.
* DAS User's Guide ([Jupyter Notebook](notebooks/das-users-guide.ipynb)) ([Markdown](docs/das-users-guide.md)) - Document with code snippets showing how to use DAS.
* [DAS Overview](docs/das-overview.md) - Description of the main DAS components and deployment architecture.
* [DAS Toolbox User's Guide](https://github.com/singnet/das-toolbox) - A guide with detailed instructions to use the DAS Server Toolbox.
* [Release Notes](docs/release-notes.md) - Release notes for all DAS components.

## **Project Management**

* [Public board](https://github.com/orgs/singnet/projects/7) - GitHub project board used to track bug reports, feature requests, and major new features planning. Use this board to report bugs or request new features.
* [Development board](https://github.com/orgs/singnet/projects/6/views/1) - Used internally by the DAS Team to track issues and tasks.
* _Contribution guidelines_
