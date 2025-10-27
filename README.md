# OpenCog Hyperon - Distributed Atomspace (DAS)

## Project

These are the Github repos we're currently using in the project:

* [Source code (this repo)](https://github.com/singnet/das) - Main DAS source code
* [Simplified MeTTa parser](https://github.com/singnet/das-metta-parser) - Simple parser to load knowledge bases (no actual MeTTa commmand interpreetation/execution)
* [Database adapter](https://github.com/singnet/das-database-adapter) - Adapter to use DBMSs as atomspaces
* [Command line tools](https://github.com/singnet/das-toolbox)  - Tools to configure and deploy DAS components
* [Protobuf definitions](https://github.com/singnet/das-proto) - Remote API used all along DAS' components
* [Github actions library](https://github.com/singnet/das-cicd) - CI/CD scripts used in DAS repos
* [NuNET/Rejuve PoC](https://github.com/singnet/das-nunet-rejuve-poc) - Simple use case to assess DAS cognitive capabilities
* Vultr [pre-infra](https://github.com/singnet/das-pre-infra-vultr) and [infra](https://github.com/singnet/das-infra-stack-vultr) - Infra deployment/setup in Vultr
* AWS [pre-infra](https://github.com/singnet/das-pre-infra-aws) and [infra](https://github.com/singnet/das-infra-stack-aws) - Infra deployment/setup in AWS

These repos are frozen and deprecated:

* https://github.com/singnet/das-atom-db
* https://github.com/singnet/das-query-engine
* https://github.com/singnet/das-serverless-functions
* https://github.com/singnet/das-attention-broker
* https://github.com/singnet/das-node
* https://github.com/singnet/das-postgres-lobe
* https://github.com/singnet/das-openfaas-templates
* https://github.com/singnet/das-poc

Issues for project management are kept in two boards:

* [Public board](https://github.com/orgs/singnet/projects/7) - GitHub project board used to track bug reports, feature requests, and major new features planning. Use this board to report bugs or request new features.
* [Development board](https://github.com/orgs/singnet/projects/6/views/1) - Used internally by the DAS Team for day-by-day work planning.

## DAS Usage

DAS (Distributed AtomSpace) is an extension to Hyperon that allows the use of
very large knowledge bases by offering a query API (fully integrated with
MeTTa) that uses several internal cognitive components to improve query
execution performance. The actual backend used to persist the knowledge base can
be configured to use any DBMS or combination of DBMSs.

Currently, DAS is integrated with [Hyperon Experimental](https://github.com/trueagi-io/hyperon-experimental/)
and can be used as a special type of space. Integration with other MeTTa interpreters
is possible and are being planned for the near future.

In order to use DAS in Hyperon Experimental, you need to install our
configuration command-line tool,
[das-cli](https://github.com/singnet/das-toolbox/blob/master/das-cli/README.md),
and to use it to configure the DAS components you are planning to use:

* AtomDB
* Query Engine
* Evolution agent
* Link Creation agent
* Inference agent

In [this README](https://github.com/singnet/das/blob/master/docs/das-cli-users-guide.md) we provide details and examples
of the various commands we use here to configure DAS. Please refer to it to better understand the command line parameters
we mention below. `das-cli` also has `man pages` and an embedded documentation (invoked with `--help`) which are installed
together with the toolbox package.

In addition to this documentation, we also have a couple of [Jupyter](https://jupyter.org) notebooks with hands-on examples
of how to configure DAS and use it inside Hyperon Experimental. These notebook can be found 
[here](https://github.com/singnet/das/tree/master/notebooks).

### 1. Configuring your DAS environment

To configure your DAS environment, just run

```bash
das-cli config set
```

If you are setting up a new environment, it's safe to hit `<ENTER>` and accept all suggested default values. Just make sure
you don't have any other tool listening to the PORT numbers we are using.

### 2. Starting an AtomDB

The AtomDB is where the knowlege base is persisted. If you already have a persisted AtomDB you can just
start the Atom DB by calling

```bash
das-cli db start
```

If you don't have your knowledge base persisted as an AtomDB yet, you need to load it as a MeTTa file by calling

```bash
das-cli db start
das-cli metta load /absolut/path/to/test/metta/file/at/das/src/tests/animals_extended.metta
```

You're supposed to see an output like this:

```tty
$ das-cli db start
Starting Redis service...
Redis has started successfully on port 29000 at localhost, operating under the server user senna.
Starting MongoDB service...
MongoDB has started successfully on port 28000 at localhost, operating under the server user senna.
$ das-cli metta load /absolut/path/to/test/metta/file/at/das/src/tests/animals_extended.metta
das-cli-mongodb-28000 is running on port 28000
das-cli-redis-29000 is running on port 29000
Loading metta file /tmp/animals_extended.metta...
Connecting to Redis at localhost:29000
Connecting to MongoDB at localhost:28000
Done.
```

Optionally, we support the usage of some backends directly without the need to
export the data to MeTTa and load them in DAS. It means you can use the DBMS server directly
to feed the AtomDB. Newly created atoms are not sent back to the DBMS yet but this is a planned
feature for the (near to) mid future.

Currently we support [Postgresql](https://www.postgresql.org) and 
[Mork](https://github.com/trueagi-io/MORK/blob/server/README.md) but we are working to extend this list. 
Look [here](https://github.com/singnet/das/blob/master/docs/database-adapter-users-guide.md)
for complete documentation on how to use our Database Adapter.

### 3. Starting the AttentionBroker

[das-cli](https://github.com/singnet/das-toolbox) can be used to start/stop any
of the DAS components. So you should use it to start the AttentionBroker.

```bash
das-cli attention-broker start
```

You're supposed to see an output like this:

```tty
$ das_cli.py attention-broker start
Starting Attention Broker service...
Attention Broker started on port 40001
```

### 4. Starting Query Engine and other agents

Similarly, you can use `das-cli` to start the Query Engine and the other DAS agents. The Query Engine is
a mandatory component but the other agents are optional.

```bash
das-cli query-agent start
das-cli evolution-agent start
das-cli link-creation-agent start
das-cli inference-agent start
```

## Contributing with DAS

If you want to contribute with DAS project, please read our 
[Development Guidelines](https://github.com/singnet/das/blob/master/docs/developer_guidelines.md) first.


