# Hyperon Distributed Atom Space Algorithm Node

The Hyperon Distributed Atom Space Algorithm Node (aka DAS Node) is a core
component of the Distributed AtomSpace (DAS)
(<https://github.com/singnet/das>), enabiling the implementation of distributed
algorithms across DAS instances using one or more shared knowledge base. This
documentation provides an overview of the DAS Node's purpose, architecture, and
usage, catering to users with no prior knowledge of the system.


## Introduction

Remote DAS queries will be triggered by a specialization of our
DistributedALgorithmNode which will connect client (query caller) and server
(DAS Server running a Redis-mongo atomdb) using command lines (this is the way
AtomSpaceNode exchange messages

<!--TODO: Add a chart representing an arbitrary number of DAS Node's quering on
an arbitrary number of DAS Servers-->

## Key Concepts and Components

### Messages

Communication between nodes is done through Message objects. These objects act
like commands exchanged between nodes, although the actual code isn't
transmitted. Instead, a command identifier is sent, and the receiving node
reconstructs and executes the corresponding Message object. The command format
resembles a command-line interface, with commands and arguments.

Message is an abstract class representing messages exchanged between nodes.
Each subclass should implement the act() method, which defines the behavior
that will be executed on the receiving node.

`Messages` are exchanged through a `MessageBroker`, which implements which
communication protocol we will be using. Currently there are the following
implementations available:

- SynchronousSharedRAM
- SynchronousGRPC
- AsynchronousMQTT

### Leadership

In distributed algorithms, a common problem is coordination, there are a number
of known algorithms to solve for that such as Raft, Paxos among others.
LeadershipBroker defines the API for leader election in the network. Users
typically don't interact with this class directly; it's managed by the
DistributedAlgorithmNode.

### DistributedAlgorithmNode

DistributedAlgorithmNode is an abstract class that represents a node in a
network, designed for distributed processing. Users must extend this class to
implement custom distributed networks.

Key Points for Extending DistributedAlgorithmNode:

- DistributedAlgorithmNode builds Message objects because it inherits from
MessageFactory. The base class can create basic Message objects for common
tasks, such as handling new nodes joining the network. However, subclasses
should override message_factory() to handle application-specific messages.

  - Message execution is threaded. If commands update shared state in
  DistributedAlgorithmNode or its subclasses, you must protect this state using
  mutual exclusion mechanisms.

  - The constructor for DistributedAlgorithmNode requires a MessageBroker and a
  LeadershipBroker, both of which are abstract. You must choose concrete
  implementations or create your own, depending on the communication and
  leadership election strategies you want to use. Custom leadership algorithms
  may be needed, depending on the network topology and application
  requirements.

  - DistributedAlgorithmNode has several pure virtual methods that must be
  implemented by subclasses. These methods handle fundamental tasks, such as
  leadership elections and notifying nodes when new peers join the network.

## Examples

We provide bindings to Python to ease usability and fast prototyping.
Import the python package and create your own DistributedAlgorithmNode:

```python
from hyperon_das_node import (
  DistributedAlgorithmNode,
  LeadershipBrokerType,
  MessageBrokerType
)

class MyDasNode(DistributedAlgorithmNode):
  def __init__(self, node_id):
    super().__init__(
      node_id,
      LeadershipBrokerType.SINGLE_MASTER_SERVER,
      MessageBrokerType.GRPC,
    )

  def message_factory(self, command: str, args: list[str]) -> Message:
    message = super().message_factory(command, args)
    if message is not None:
      return Message

    # Create you custom Messages objects here
    ...

```

Inside the examples folder you can find a number of examples of how to use the
library. The `simple_node.py` shows a very simple example of how to use the
library.


## Building from source

The build process involves two steps: first building the C++ code with Bazel,
and then building and installing the Python package.

We are using a docker image with `bazel` installed. To build the image you can
run:

```sh
make cpp-image
make cpp-build
make wheel-image
make wheel-build
```

Once the image is built and the code is built, all the libraries will be copied
to a folder named `bazel_assets`. This folder will be used to build the python
package.

To install the package inside the docker container, you can run: ```sh pip
install . # This allows to run tests in python inside the container. ```

```sh
make install
```

