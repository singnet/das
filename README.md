# OpenCog Hyperon - Distributed Atomspace (DAS)

This repository contains items relevant to the DAS project that do not belong in any other project's specific repositories, such as automated deployment scripts, API and high-level documentation, and assets used by other repositories' CI/CD scripts.

## **Components**

DAS consists of several components. Below is a list of these components:

| **Component**             | **Description**                                                                 | **Documentation Link**                                             |
|---------------------------|---------------------------------------------------------------------------------|-------------------------------------------------------------------|
| **Query Agent**            | A data manipulation API for Distributed Atomspace (DAS). It allows queries with pattern matching capabilities and traversal of the Atomspace hypergraph. | - [Query Agent README](query_agent/python/README.md) |
| **Attention Broker**       | DAS component that keeps track of atom's importance values attached to different contexts and updates those values according to the queries made by users using context-specific Hebbian networks. | [Attention Broker README](attention_broker/README.md)       |
| **Link Creation Agent**    | | [Link Creation README](shared/README.md)                           |
| **Database Wrapper**       |                                                    | [Scripts README](scripts/README.md)                                |
| **Inference Control**      |  | [Scripts README](scripts/README.md)                                |
| **Atom DB**                | Source code for hyperon-das-atomdb, a Python library required by hyperon-das to interface with DBMSs and other data storage structures. | [Atom DB README](atom_db/README.md)                                |
| **DAS Server** | Source code of client/server communication components in the DAS Server FaaS architecture. | [Serverless Functions README](serverless_functions/README.md) <br> [OpenFaaS Templates README](openfaas_templates/README.md) |
| **DAS MeTTa Parser**       | A simplified MeTTa parser used to feed knowledge bases into DAS.                  | [DAS MeTTa Parser README](das_metta_parser/README.md)              |
| **DAS Server Toolbox**     | A set of tools to deploy and set up a DAS server.                               | [DAS Server Toolbox README](https://github.com/singnet/das-toolbox)          |
| **Infrastructure Setup**   | Tools and assets required to provision and set up servers in cloud providers.    | - [Server provisioning in Vultr](infrastructure/vultr/provisioning/README.md) <br> - [Server setup Vultr](infrastructure/vultr/setup/README.md) <br> - [Server provisioning in AWS](infrastructure/aws/provisioning/README.md) <br> - [Server setup AWS](infrastructure/aws/setup/README.md) |                            |


## **Getting Started**

### **1. Cloning the Repository**
```bash
git clone <repo-url>
cd monorepo
```

### **2. Build**

To build the agents, run the command:

```bash
make build
```

This will generate the binaries for all components in `das/src/bin`.

## **Documentation**

* [DAS API](https://singnet.github.io/das-query-engine/api/DAS/) - Documentation for the DAS API.
* DAS User's Guide ([Jupyter Notebook](notebooks/das-users-guide.ipynb)) ([Markdown](docs/das-users-guide.md)) - Document with code snippets showing how to use DAS.
* [DAS Overview](docs/das-overview.md) - Description of the main DAS components and deployment architecture.
* [DAS Toolbox User's Guide](https://github.com/singnet/das-toolbox) - A guide with detailed instructions to use the DAS Server Toolbox.
* [Release Notes](docs/release-notes.md) - Release notes for all DAS components.

## Project Management

* [Public board](https://github.com/orgs/singnet/projects/7) - GitHub project board used to track bug reports, feature requests, and major new features planning. Use this board to report bugs or request new features.
* [Development board](https://github.com/orgs/singnet/projects/6/views/1) - Used internally by the DAS Team to track issues and tasks.
* _Contribution guidelines_
