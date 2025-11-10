# DAS - Jupyter Example Notebooks

This directory contains example Jupyter notebooks that showcase how to set up the Distributed Atomspace (DAS) environment and run different queries against it using [hyperon-experimental](https://github.com/trueagi-io/hyperon-experimental) python package.

Use these notebooks as step-by-step guides for bootstrapping services, loading sample datasets, and experimenting with DAS workflows.

- [das-he-setup.ipynb](./das-he-setup.ipynb) — Walks through installing prerequisites, starting DAS services (Databases, Attention Broker, Query Agent, Context Broker, Evolution, Link Creation, and Inference agents), and loading the sample animal knowledge base used by the other notebooks.

- [das-he-animals.ipynb](./das-he-animals.ipynb) — Shows pattern-matching queries over the sample animals knowledge, including matches, logical combinations (AND/OR), and nested patterns using the DAS Query Agent.

- [das-he-lca.ipynb](./das-he-lca.ipynb) — Introduces the Link Creation Agent, showing how to configure queries, templates, and built-in processors to synthesize new links from existing DAS knowledge.

- [das-he-sentences-evolution.ipynb](./das-he-sentences-evolution.ipynb) — Explores evolution-based queries on a large sentences dataset, covering context creation, fitness functions, and iterative optimization with the Evolution Agent.
