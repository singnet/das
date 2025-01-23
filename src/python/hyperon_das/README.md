# Hyperon DAS

A data manipulation API for Distributed Atomspace (DAS). It allows queries with pattern matching capabilities and traversal of the Atomspace hypergraph.

## References and Guides

- Details about the Distributed Atomspace and its components: [DAS Overview](https://singnet.github.io/das/das-overview)
- PYPI Library package: [hyperon-das](https://pypi.org/project/hyperon-das/)
- Examples using the API: [User's Guide](https://singnet.github.io/das/das-users-guide) 
- Release notes: [DAS Query Engine Releases](https://github.com/singnet/das-query-engine/releases)

## Installation

> Before you start, make sure you have [Python](https://www.python.org/) >= 3.10 and [Pip](https://pypi.org/project/pip/) installed on your system.

You can install and run this project using different methods. Choose the one that suits your needs.

### Using-pip

Run the following command to install the project using pip:: 

```bash
pip install hyperon-das
```

### Using-Poetry

If you prefer to manage your Python projects with [Poetry](https://python-poetry.org/), follow these steps:

1.    
    Install Poetry (if you haven't already):
    
    ```bash
    pip install poetry
    ``` 
    
2.  Clone the project repository:
    
    ```bash
    git clone git@github.com:singnet/das-query-engine.git
    cd das-query-engine
    ``` 
    
3.  Install project dependencies using Poetry:
    
    ```bash
    poetry install
    ```
    Note: If perhaps you are running over SSH, poetry install might stuck checking the keyring, you can verify this by running `poetry install -vvv`, then the command will be stuck on the following lines:
    ```bash
    Checking if keyring is available
    [keyring:keyring. backend] Loading KWallet |  
    [keyring:keyring.backend] Loading SecretService |  
    [keyring:keyring. backend] Loading Windows |  
    [keyring: keyring.backend] Loading chainer |  
    [keyring:keyring.backend] Loading libsecret |  
    [keyring:keyring.backend] Loading macOS |  
    Using keyring backend 'SecretService Keyring'
    ```

    If that is the case, deactivate keyring and run poetry install again:
    ```bash
    poetry config keyring.enabled false
    poetry install
    ```
    
5.  Activate the virtual environment created by Poetry:
    
    ```bash
    poetry shell
    ``` 

Now you can run the project within the Poetry virtual environment.

## Tests

In the main project directory, you can run the command below to run the unit tests

```bash
make unit-tests
```

Likewise, to run performance tests

```bash
make performance-tests
```
Generating atoms and checking the performance.
This test typically takes more than 60 seconds to run with the default settings.
Arguments allowed in OPTIONS:
- `--node_count` (default: "100"): Number of nodes in the knowledge base
- `--word_count` (default: "8"): Number of words in a node's name
- `--word_length` (default: "3"): Number of characters in each word of node's name
- `--alphabet_range` (default: "2-5"): Determines the range for the alphabet size.
- `--word_link_percentage` (default: 0.1): Percentage of word links.
- `--letter_link_percentage` (default: 0.1): Percentage of letter links.
- `--seed` (default: 11): Sets the random seed for reproducibility (int/float).
- `--repeat` (default: 1): (Test only) Repeats test n times to collect average/std deviation of execution time.
- `--mongo_host_port` (default: "localhost:15927"): (Test only) Mongo hostname and port. eg: localhost:1234.
- `--mongo_credentials` (default: \*\*\*:\*\*\* ): (Test only) Mongo username and password. eg: user:pass.
- `--redis_host_port` (default: "localhost:15926"): (Test only) Redis hostname and port. eg: localhost:1234.
- `--redis_credentials` (default: ":"): (Test only) Redis username and password. eg: user:pass.
- `--redis_cluster` (default: False): (Test only) Redis cluster configuration.
- `--redis_ssl` (default: False): (Test only) Sets Redis SSL.

```bash
make benchmark-tests OPTIONS="--word_link_percentage=0.01"
```
or create a MeTTa file using the same options:
```bash
make benchmark-tests-metta-file OPTIONS="--word_link_percentage=0.01"
```


You can do the same to run integration tests

Arguments allowed in OPTIONS:
- `--no-destroy` (default: False): Prevents the test container from being destroyed after the test ends, 
allowing for faster subsequent runs by reusing the same container.
- `--build` (default: False): Rebuilds the container image, use only if the Dockerfile has changed, as rebuilding the 
container image can take a significant amount of time.

```bash
make integration-tests
```
or
```bash
make integration-tests OPTIONS="--no-destroy"
```

The integration tests use a remote testing server hosted on Vultr, at the address `45.63.85.59`, port `8080`. The loaded knowledge base is the animal base, which contains the Nodes and Links listed below:

```text
(: Similarity Type)
(: Concept Type)
(: Inheritance Type)
(: "human" Concept)
(: "monkey" Concept)
(: "chimp" Concept)
(: "snake" Concept)
(: "earthworm" Concept)
(: "rhino" Concept)
(: "triceratops" Concept)
(: "vine" Concept)
(: "ent" Concept)
(: "mammal" Concept)
(: "animal" Concept)
(: "reptile" Concept)
(: "dinosaur" Concept)
(: "plant" Concept)
(Similarity "human" "monkey")
(Similarity "human" "chimp")
(Similarity "chimp" "monkey")
(Similarity "snake" "earthworm")
(Similarity "rhino" "triceratops")
(Similarity "snake" "vine")
(Similarity "human" "ent")
(Inheritance "human" "mammal")
(Inheritance "monkey" "mammal")
(Inheritance "chimp" "mammal")
(Inheritance "mammal" "animal")
(Inheritance "reptile" "animal")
(Inheritance "snake" "reptile")
(Inheritance "dinosaur" "reptile")
(Inheritance "triceratops" "dinosaur")
(Inheritance "earthworm" "animal")
(Inheritance "rhino" "mammal")
(Inheritance "vine" "plant")
(Inheritance "ent" "plant")
(Similarity "monkey" "human")
(Similarity "chimp" "human")
(Similarity "monkey" "chimp")
(Similarity "earthworm" "snake")
(Similarity "triceratops" "rhino")
(Similarity "vine" "snake")
(Similarity "ent" "human")
```
