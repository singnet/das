# OpenCog Hyperon - Distributed Atomspace (DAS)

Here, we keep stuff that is pertinent to the DAS project but doesn't belong to
any of the other project's specific repositories, such as automated deployment
scripts, API and high level documentation, assets used by other repositories'
CI/CD scripts, etc.

## Documentation

* [DAS API](https://singnet.github.io/das-query-engine/api/das/) - Documentation of DAS API
* _DAS User's Guide_
* [DAS Overview](docs/das-overview.md) - Description of main DAS components and deployment architecture.
* [DAS Toolbox User's Guide](https://github.com/singnet/das-toolbox) - A guide with detailed instructions to use the DAS Server Toolbox.
* [Release Notes](docs/release-notes.md) - Release notes of all DAS components.

## Repositories

In addition to this one, we use many other repositories in the project.

* [Query Engine](https://github.com/singnet/das-query-engine) - Source code for [hyperon-das](https://pypi.org/project/hyperon-das/), the Python library with the main [DAS API](https://singnet.github.io/das-query-engine/api/das/).
* [Atom DB](https://github.com/singnet/das-atom-db) - Source code for [hyperon-das-atomdb](https://pypi.org/project/hyperon-das-atomdb/), a Python library required by hyperon-das to interface with DBMSs and other data storage structures.
* [Serverless Functions](https://github.com/singnet/das-serverless-functions) and [OpenFaaS Templates](https://github.com/singnet/das-toolbox) - Source code of client/server communication components in DAS Server FaaS architecture.
* [DAS MeTTa Parser](https://github.com/singnet/das-metta-parser) - A simplified MeTTa parser used to feed knowledge bases into DAS.
* [DAS Server Toolbox](https://github.com/singnet/das-toolbox) - A set of tools to deploy and setup a DAS server.
* Infrastructure setup - Tools and assets required to provision and setup servers in cloud providers.
    * [Server provisioning in Vultr](https://github.com/singnet/das-pre-infra-vultr)
    * [Server setup Vultr](https://github.com/singnet/das-infra-stack-vultr)
    * [Server provisioning in AWS](https://github.com/singnet/das-pre-infra-aws)
    * [Server setup AWS](https://github.com/singnet/das-infra-stack-aws)

## Project management

* [Public board](https://github.com/orgs/singnet/projects/7) - GitHub project board used to track bug reports, feature requests and major new features planning. Use this board to report bugs or request new features.
* [Development board](https://github.com/orgs/singnet/projects/6/views/1) - Used internally by the DAS Team to track issues and tasks.
* _Contribution guidelines_
