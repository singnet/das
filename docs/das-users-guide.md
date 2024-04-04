# DAS - User's Guide

Atomspace is the hypergraph OpenCog Hyperon uses to represent and store knowledge, being the source of knowledge for AI agents and the container of any computational result that might be created or achieved during their execution.

The __Distributed Atomspace (DAS)__ is an extension of OpenCog Hyperon's Atomspace into a more independent component designed to support multiple simultaneous connections with different AI algorithms, providing a flexible query interface to distributed knowledge bases. It can be used as a component (e.g. a Python library) or as a stand-alone server to store essentially arbitrarily large knowledge bases and provide means for the agents to traverse regions of the hypergraphs and perform global queries involving properties, connectivity, subgraph topology, etc.

Regardless of being used locally or remotely, DAS provides the exact same API to query or traverse the Atomspace. This API is fully documented [here](https://singnet.github.io/das-query-engine/api/das/). In this document we provide examples and best practices to use this API with each type of DAS.

## Table of contents

* [Local DAS with data in RAM](#ramdas)
    * [Adding atoms](#addatoms)
    * [Fetching from a DAS server](#fetch)
    * [Getting atoms by their properties](#atomquery)
    * [Traversing the hypergraph](#traversing)
    * [Pattern Matcher Queries](#patternmatcher)
* [Connecting to a remote DAS](#remotedas)
    * [Querying a remote DAS](#remotequery)
    * [Custom indexes](#customindex)
* [Starting a DAS Server](#dasserver)

<a id='ramdas'></a>
## Local DAS with data in RAM


A local DAS stores atoms as Python dicts in RAM. One can create an empty DAS by calling the basic constructor with no parameters.


```python
from hyperon_das import DistributedAtomSpace

das = DistributedAtomSpace()
print(das.count_atoms())
```

    (0, 0)


This is equivalent to instantiating it passing `query_engine='local'`


```python
from hyperon_das import DistributedAtomSpace

das = DistributedAtomSpace(query_engine='local')
print(das.count_atoms())
```

    (0, 0)


The `query_engine` parameter is used to select from `local` or `remote` DAS. [Remote DAS](#remotedas) is explained later in this document.

A local DAS can be populated using the methods `add_node()` and `add_link()`. Let's use this simple knowledge base as an example.

<p align="center">
<img src="../docs/assets/pmquery_1.png" width="400"/>
</p>


We have only one type of node (e.g. Concept) to represent animals and two types of links, (e.g. Inheritance and Similarity) to represent relations between them.

<a id='addatoms'></a>
### Adding atoms

We can add nodes explicitly by calling `add_node()` passing a Python dict representing the node. This dict may contain any number of keys associated to values of any type (including lists, sets, nested dicts, etc) , which are all recorded with the node, but must contain at least the keys `type` and `name` mapping to strings which define the node uniquely, i.e. two nodes with the same `type` and `name` are considered to be the same entity.


```python
    das.add_node({"type": "Concept", "name": "human"})
    das.add_node({"type": "Concept", "name": "monkey"})
    das.add_node({"type": "Concept", "name": "chimp"})
    das.add_node({"type": "Concept", "name": "mammal"})
    das.add_node({"type": "Concept", "name": "reptile"})
    das.add_node({"type": "Concept", "name": "snake"})
    das.add_node({"type": "Concept", "name": "dinosaur"})
    das.add_node({"type": "Concept", "name": "triceratops"})
    das.add_node({"type": "Concept", "name": "earthworm"})
    das.add_node({"type": "Concept", "name": "rhino"})
    das.add_node({"type": "Concept", "name": "vine"})
    das.add_node({"type": "Concept", "name": "ent"})
    das.add_node({"type": "Concept", "name": "animal"})
    das.add_node({"type": "Concept", "name": "plant"}) ;
```

We can also add nodes implicitly while adding links.


```python
das.add_link(
    {
        "type": "Similarity",
        "targets": [
            {"type": "Concept", "name": "human"},
            {"type": "Concept", "name": "monkey"},
        ],
    }
) ;
```

"human" and "monkey" would be inserted if they hadn't been inserted before. Adding the node or link more than once is allowed and has no side effects. So let's add the whole set of links from our knowledge base.


```python
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "human"}, {"type": "Concept", "name": "monkey"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "human"}, {"type": "Concept", "name": "chimp"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "chimp"}, {"type": "Concept", "name": "monkey"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "snake"}, {"type": "Concept", "name": "earthworm"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "rhino"}, {"type": "Concept", "name": "triceratops"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "snake"}, {"type": "Concept", "name": "vine"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "human"}, {"type": "Concept", "name": "ent"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "monkey"}, {"type": "Concept", "name": "human"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "chimp"}, {"type": "Concept", "name": "human"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "monkey"}, {"type": "Concept", "name": "chimp"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "earthworm"}, {"type": "Concept", "name": "snake"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "triceratops"}, {"type": "Concept", "name": "rhino"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "vine"}, {"type": "Concept", "name": "snake"}, ], })
das.add_link( { "type": "Similarity", "targets": [ {"type": "Concept", "name": "ent"}, {"type": "Concept", "name": "human"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "human"}, {"type": "Concept", "name": "mammal"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "monkey"}, {"type": "Concept", "name": "mammal"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "chimp"}, {"type": "Concept", "name": "mammal"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "mammal"}, {"type": "Concept", "name": "animal"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "reptile"}, {"type": "Concept", "name": "animal"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "snake"}, {"type": "Concept", "name": "reptile"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "dinosaur"}, {"type": "Concept", "name": "reptile"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "triceratops"}, {"type": "Concept", "name": "dinosaur"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "earthworm"}, {"type": "Concept", "name": "animal"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "rhino"}, {"type": "Concept", "name": "mammal"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "vine"}, {"type": "Concept", "name": "plant"}, ], })
das.add_link( { "type": "Inheritance", "targets": [ {"type": "Concept", "name": "ent"}, {"type": "Concept", "name": "plant"}, ], }) ;
```

Links are always asymetric, so symmetric relationships like "Similarity" are represented by adding two links. For instance:
    
```
das.add_link(
    {
        "type": "Similarity",
        "targets": [
            {"type": "Concept", "name": "human"},
            {"type": "Concept", "name": "monkey"},
        ],
    }
)
```

and

```
das.add_link(
    {
        "type": "Similarity",
        "targets": [
            {"type": "Concept", "name": "monkey"},
            {"type": "Concept", "name": "human"},
        ],
    }
)
```

Considering this, we can print the atom count again.


```python
print(das.count_atoms())
```

    (14, 26)


<a id='fetch'></a>
### Fetching from a DAS server

Instead of adding atoms by calling `add_node()` and `add_link()` directly, it's possible to fetch all or part of the contents from a DAS server using the method `fetch()`. This method doesn't create a lasting connection with the DAS server, it will just fetch the atoms once and close the connection so any subsequent changes or queries will not be propagated to the server in any way. After fetching the atoms, all queries will be made locally. It's possible to call `fetch()` multiple times fetching from the same DAS Server or from different ones.


```python
from hyperon_das import DistributedAtomSpace

remote_das_host = "45.63.85.59"
remote_das_port = 8080

imported_das = DistributedAtomSpace()
print(imported_das.count_atoms())

links_to_import = {
    'atom_type': 'link',
    'type': 'Expression',
    'targets': [
        {'atom_type': 'node', 'type': 'Symbol', 'name': 'Inheritance'},
        {'atom_type': 'variable', 'name': 'v2'},
        {'atom_type': 'variable', 'name': 'v3'},
    ]
}

imported_das.fetch(links_to_import, remote_das_host, remote_das_port)
print(imported_das.count_atoms())
```

    (0, 0)
    (15, 12)


The first parameter of `fetch()` is a pattern to describe which atoms should be fetched. It's exactly the same pattern used to make [pattern matching](#patternmatcher).

<a id='atomquery'></a>
### Getting atoms by their properties

DAS has an API to query atoms by their properties. Most of this API is based on atom handles. Handles are MD5 signatures associated with atoms. For now they are supposed to be unique ids for atoms although this is not 100% true (conflict handling is planned to be implemented in the near future). DAS provides two static methods to compute handles for nodes and links: `das.get_node_handle()` and `das.get_link_handle()`.


```python
human = das.get_node_handle('Concept', 'human')
ent = das.get_node_handle('Concept', 'ent')

print("human:", human)
print("ent:", ent)

similarity_link = das.get_link_handle('Similarity', [human, ent])

print("Similarity link:", similarity_link)
```

    human: af12f10f9ae2002a1607ba0b47ba8407
    ent: 4e8e26e3276af8a5c2ac2cc2dc95c6d2
    Similarity link: 16f7e407087bfa0b35b13d13a1aadcae


Note that these are static methods which don't actually query the stored atomspace in order to compute those handles. Instead, they just run a MD5 hashing algorithm over the data that uniquely identifies nodes and links, i.e. node type and name in the case of nodes and link type and targets in the case of links. This means e.g. that two nodes with the same type and the same name are considered to be the exact same entity.

Atom handles can be used to retrieve the actual atom document.


```python
das.get_atom(human)
```




    {'handle': 'af12f10f9ae2002a1607ba0b47ba8407',
     'type': 'Concept',
     'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3',
     'name': 'human',
     'named_type': 'Concept'}



Convenience methods can be used to retrieve atoms passing its basic properties instead.


```python
print("human:", das.get_node('Concept', 'human'))
print("\nSimilarity link:", das.get_link('Similarity', [human, ent]))
```

    human: {'handle': 'af12f10f9ae2002a1607ba0b47ba8407', 'type': 'Concept', 'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3', 'name': 'human', 'named_type': 'Concept'}
    
    Similarity link: {'handle': '16f7e407087bfa0b35b13d13a1aadcae', 'type': 'Similarity', 'composite_type_hash': 'ed73ea081d170e1d89fc950820ce1cee', 'is_toplevel': True, 'composite_type': ['a9dea78180588431ec64d6bc4872fdbc', 'd99a604c79ce3c2e76a2f43488d5d4c3', 'd99a604c79ce3c2e76a2f43488d5d4c3'], 'named_type': 'Similarity', 'named_type_hash': 'a9dea78180588431ec64d6bc4872fdbc', 'targets': ['af12f10f9ae2002a1607ba0b47ba8407', '4e8e26e3276af8a5c2ac2cc2dc95c6d2']}


It's possible to get all links pointing to a specific atom.


```python
# All links pointing from/to 'rhino'

rhino = das.get_node_handle('Concept', 'rhino')
links = das.get_incoming_links(rhino)
for link in links:
    print(link['type'], link['targets'])
```

    Similarity ['d03e59654221c1e8fcda404fd5c8d6cb', '99d18c702e813b07260baf577c60c455']
    Similarity ['99d18c702e813b07260baf577c60c455', 'd03e59654221c1e8fcda404fd5c8d6cb']
    Inheritance ['99d18c702e813b07260baf577c60c455', 'bdfe4e7a431f73386f37c6448afe5840']


Links can also be retrieved by other properties or partial definition of its main properties (type and targets). The method `get_links()` can be used passing different combinations of parameters.


```python
# All inheritance links

links = das.get_links(link_type='Inheritance')
for link in links:
    print(link['type'], link['targets'])                      
```

    Inheritance ['5b34c54bee150c04f9fa584b899dc030', 'bdfe4e7a431f73386f37c6448afe5840']
    Inheritance ['b94941d8cd1c0ee4ad3dd3dcab52b964', '80aff30094874e75028033a38ce677bb']
    Inheritance ['bb34ce95f161a6b37ff54b3d4c817857', '0a32b476852eeb954979b87f5f6cb7af']
    Inheritance ['c1db9b517073e51eb7ef6fed608ec204', 'b99ae727c787f1b13b452fd4c9ce1b9a']
    Inheritance ['bdfe4e7a431f73386f37c6448afe5840', '0a32b476852eeb954979b87f5f6cb7af']
    Inheritance ['1cdffc6b0b89ff41d68bec237481d1e1', 'bdfe4e7a431f73386f37c6448afe5840']
    Inheritance ['af12f10f9ae2002a1607ba0b47ba8407', 'bdfe4e7a431f73386f37c6448afe5840']
    Inheritance ['b99ae727c787f1b13b452fd4c9ce1b9a', '0a32b476852eeb954979b87f5f6cb7af']
    Inheritance ['4e8e26e3276af8a5c2ac2cc2dc95c6d2', '80aff30094874e75028033a38ce677bb']
    Inheritance ['d03e59654221c1e8fcda404fd5c8d6cb', '08126b066d32ee37743e255a2558cccd']
    Inheritance ['99d18c702e813b07260baf577c60c455', 'bdfe4e7a431f73386f37c6448afe5840']
    Inheritance ['08126b066d32ee37743e255a2558cccd', 'b99ae727c787f1b13b452fd4c9ce1b9a']



```python
# Inheritance links between two Concept nodes

links = das.get_links(link_type='Inheritance', target_types=['Concept', 'Concept'])
for link in links:
    print(link['type'], link['targets'])   
```

    Inheritance ['5b34c54bee150c04f9fa584b899dc030', 'bdfe4e7a431f73386f37c6448afe5840']
    Inheritance ['b94941d8cd1c0ee4ad3dd3dcab52b964', '80aff30094874e75028033a38ce677bb']
    Inheritance ['bb34ce95f161a6b37ff54b3d4c817857', '0a32b476852eeb954979b87f5f6cb7af']
    Inheritance ['c1db9b517073e51eb7ef6fed608ec204', 'b99ae727c787f1b13b452fd4c9ce1b9a']
    Inheritance ['bdfe4e7a431f73386f37c6448afe5840', '0a32b476852eeb954979b87f5f6cb7af']
    Inheritance ['1cdffc6b0b89ff41d68bec237481d1e1', 'bdfe4e7a431f73386f37c6448afe5840']
    Inheritance ['af12f10f9ae2002a1607ba0b47ba8407', 'bdfe4e7a431f73386f37c6448afe5840']
    Inheritance ['b99ae727c787f1b13b452fd4c9ce1b9a', '0a32b476852eeb954979b87f5f6cb7af']
    Inheritance ['4e8e26e3276af8a5c2ac2cc2dc95c6d2', '80aff30094874e75028033a38ce677bb']
    Inheritance ['d03e59654221c1e8fcda404fd5c8d6cb', '08126b066d32ee37743e255a2558cccd']
    Inheritance ['99d18c702e813b07260baf577c60c455', 'bdfe4e7a431f73386f37c6448afe5840']
    Inheritance ['08126b066d32ee37743e255a2558cccd', 'b99ae727c787f1b13b452fd4c9ce1b9a']



```python
# Similarity links where 'snake' is the first target

snake = das.get_node_handle('Concept', 'snake')
links = das.get_links(link_type='Similarity', link_targets=[snake, '*'])
for link in links:
    print(link['type'], link['targets']) 
```

    Similarity ['c1db9b517073e51eb7ef6fed608ec204', 'b94941d8cd1c0ee4ad3dd3dcab52b964']
    Similarity ['c1db9b517073e51eb7ef6fed608ec204', 'bb34ce95f161a6b37ff54b3d4c817857']



```python
# Any links where 'snake' is the first target

snake = das.get_node_handle('Concept', 'snake')
links = das.get_links(link_type='*', link_targets=[snake, '*'])
for link in links:
    print(link['type'], link['targets']) 
```

    Similarity ['c1db9b517073e51eb7ef6fed608ec204', 'b94941d8cd1c0ee4ad3dd3dcab52b964']
    Inheritance ['c1db9b517073e51eb7ef6fed608ec204', 'b99ae727c787f1b13b452fd4c9ce1b9a']
    Similarity ['c1db9b517073e51eb7ef6fed608ec204', 'bb34ce95f161a6b37ff54b3d4c817857']


<a id='traversing'></a>
### Traversing the hypergraph

It's possible to traverse the hypergraph using a `TraverseEngine` which is like a cursor that can be moved through nodes and links. First, let's initiate a `TraverseEngine` pointing to "human". In order to do this, we need to call `get_traversal_cursor()` passing the handle of the atom to be used as the starting point for the traversing. This atom can be either a link or a node. We'll use the method `das.get_node_handle()` to get the handle of the Concept "human" and start on it.


```python
cursor = das.get_traversal_cursor(das.get_node_handle('Concept', 'human'))
```

Once we have a cursor we can get the whole document of the atom pointed by it:


```python
cursor.get()
```




    {'handle': 'af12f10f9ae2002a1607ba0b47ba8407',
     'type': 'Concept',
     'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3',
     'name': 'human',
     'named_type': 'Concept'}



We can also see all links that make reference to cursor. Optional parameters can be used to filter which links should be considered. Here are some examples. We're printing only link type and targets to make the output cleaner.


```python
# All links pointing from/to cursor
print("All links:", [(d['type'], d['targets']) for d in cursor.get_links()])

# Only Inheritance links
print("\nInheritance links:", [(d['type'], d['targets']) for d in cursor.get_links(link_type='Inheritance')])

# Links whose first target is our cursor
print("\n'human' is first link target:", [(d['type'], d['targets']) for d in cursor.get_links(cursor_position=0)])
```

    All links: [('Similarity', ['af12f10f9ae2002a1607ba0b47ba8407', '4e8e26e3276af8a5c2ac2cc2dc95c6d2']), ('Inheritance', ['af12f10f9ae2002a1607ba0b47ba8407', 'bdfe4e7a431f73386f37c6448afe5840']), ('Similarity', ['af12f10f9ae2002a1607ba0b47ba8407', '5b34c54bee150c04f9fa584b899dc030']), ('Similarity', ['1cdffc6b0b89ff41d68bec237481d1e1', 'af12f10f9ae2002a1607ba0b47ba8407']), ('Similarity', ['4e8e26e3276af8a5c2ac2cc2dc95c6d2', 'af12f10f9ae2002a1607ba0b47ba8407']), ('Similarity', ['af12f10f9ae2002a1607ba0b47ba8407', '1cdffc6b0b89ff41d68bec237481d1e1']), ('Similarity', ['5b34c54bee150c04f9fa584b899dc030', 'af12f10f9ae2002a1607ba0b47ba8407'])]
    
    Inheritance links: [('Inheritance', ['af12f10f9ae2002a1607ba0b47ba8407', 'bdfe4e7a431f73386f37c6448afe5840'])]
    
    'human' is first link target: [('Similarity', ['af12f10f9ae2002a1607ba0b47ba8407', '4e8e26e3276af8a5c2ac2cc2dc95c6d2']), ('Inheritance', ['af12f10f9ae2002a1607ba0b47ba8407', 'bdfe4e7a431f73386f37c6448afe5840']), ('Similarity', ['af12f10f9ae2002a1607ba0b47ba8407', '5b34c54bee150c04f9fa584b899dc030']), ('Similarity', ['af12f10f9ae2002a1607ba0b47ba8407', '1cdffc6b0b89ff41d68bec237481d1e1'])]


There are other possibilities for filtering such as custom filter methods, target types, etc. They're explained in the [DAS API](https://singnet.github.io/das-query-engine/api/das/).

There are also convenience methods to get the cursor's "neighbors", which are the other atoms pointed by the links attached to the cursor. Let's investigate the neighbors of "human". Again, we can use the same filters to select which links and targets to consider in order to get the neighbors of the cursor.


```python
# All "human" neighbors
print("All neighbors:", [(d['type'], d['name']) for d in cursor.get_neighbors()])

# Only neighbors linked through Inheritance links
print("\nInheritance relations:", [(d['type'], d['name']) for d in cursor.get_neighbors(link_type='Inheritance')])

# Only neighbors that are similar to "human" (i.e. they share a Similarity link)
print("\nSimilar to 'human':", [(d['type'], d['name']) for d in cursor.get_neighbors(link_type='Similarity', cursor_position=0)])
```

    All neighbors: [('Concept', 'ent'), ('Concept', 'mammal'), ('Concept', 'chimp'), ('Concept', 'monkey')]
    
    Inheritance relations: [('Concept', 'mammal')]
    
    Similar to 'human': [('Concept', 'ent'), ('Concept', 'chimp'), ('Concept', 'monkey')]


<a id='cache'></a>
`get_links()` and `get_neighbors()` use the [DAS Cache system](https://github.com/singnet/das/blob/master/docs/das-overview.md) to sort the atoms before they are returned to the caller. In addition to this, these methods return an iterator rather than an actual list of atoms and this iterator is controlled by the cache system as well. The idea here is that atoms may have a large number of links (and consequently neighbors) attached to it so the AI/ML agent may not be interested in iterating on all of them. Atoms are presented in such a way that high importance atoms tend to be presented first while low importance atoms tend to be presented later.

We can move the cursor by following its links. 


```python
cursor = das.get_traversal_cursor(das.get_node_handle('Concept', 'human'))
cursor.follow_link()
cursor.get()
```




    {'handle': '4e8e26e3276af8a5c2ac2cc2dc95c6d2',
     'type': 'Concept',
     'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3',
     'name': 'ent',
     'named_type': 'Concept'}



`follow_link()` just gets the first link returned by `get_links()` in order to follow it and select a target. The same filters described above can be used here to constraint the links/targets that will be considered. For instance we could use the following code to get the most abstract concept (considering our Inheritance links) starting from "human".


```python
cursor = das.get_traversal_cursor(das.get_node_handle('Concept', 'human'))
base = cursor.get()['name']
while True:
    print(base)
    cursor.follow_link(link_type='Inheritance', cursor_position=0)
    if cursor.get()['name'] == base:
        break
    base = cursor.get()['name']
cursor.get()
```

    human
    mammal
    animal





    {'handle': '0a32b476852eeb954979b87f5f6cb7af',
     'type': 'Concept',
     'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3',
     'name': 'animal',
     'named_type': 'Concept'}



<a id='patternmatcher'></a>
### Pattern Matcher Queries

DAS can answer pattern matching queries. These are queries where the caller specifies a _pattern_ i.e. a boolean expression of subgraphs with nodes, links and wildcards and the engine finds every subgraph in the knowledge base that satisfies the passed expression. Patterns are a list of Python dicts describing a subgraph with wildcards.

The method `query()` expects a pattern and outputs a list of `QueryAnswer`. Each element in such a list has the variable assignment that satisfies the pattern and the subgraph which is the pattern itself rewritten using the given assignment.


```python
# This is a pattern like:
#
# Inheritance
#     v1
#     plant
#
# The expected answer is all Inheritance links whose second target == 'plant'
#
query = {
    'atom_type': 'link',
    'type': 'Inheritance',
    'targets': [
        {'atom_type': 'variable', 'name': 'v1'},
        {'atom_type': 'node', 'type': 'Concept', 'name': 'plant'},
    ]
}

for query_answer in das.query(query):
    print(query_answer.assignment)
    atom_matching_v1 = das.get_atom(query_answer.assignment.mapping['v1'])
    print("v1:", atom_matching_v1['type'], atom_matching_v1['name'])
    rewrited_query = query_answer.subgraph
    print(rewrited_query)
    print()
```

    [('v1', 'b94941d8cd1c0ee4ad3dd3dcab52b964')]
    v1: Concept vine
    {'handle': 'e4685d56969398253b6f77efd21dc347', 'type': 'Inheritance', 'composite_type_hash': '41c082428b28d7e9ea96160f7fd614ad', 'is_toplevel': True, 'composite_type': ['e40489cd1e7102e35469c937e05c8bba', 'd99a604c79ce3c2e76a2f43488d5d4c3', 'd99a604c79ce3c2e76a2f43488d5d4c3'], 'named_type': 'Inheritance', 'named_type_hash': 'e40489cd1e7102e35469c937e05c8bba', 'targets': [{'handle': 'b94941d8cd1c0ee4ad3dd3dcab52b964', 'type': 'Concept', 'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3', 'name': 'vine', 'named_type': 'Concept'}, {'handle': '80aff30094874e75028033a38ce677bb', 'type': 'Concept', 'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3', 'name': 'plant', 'named_type': 'Concept'}]}
    
    [('v1', '4e8e26e3276af8a5c2ac2cc2dc95c6d2')]
    v1: Concept ent
    {'handle': 'ee1c03e6d1f104ccd811cfbba018451a', 'type': 'Inheritance', 'composite_type_hash': '41c082428b28d7e9ea96160f7fd614ad', 'is_toplevel': True, 'composite_type': ['e40489cd1e7102e35469c937e05c8bba', 'd99a604c79ce3c2e76a2f43488d5d4c3', 'd99a604c79ce3c2e76a2f43488d5d4c3'], 'named_type': 'Inheritance', 'named_type_hash': 'e40489cd1e7102e35469c937e05c8bba', 'targets': [{'handle': '4e8e26e3276af8a5c2ac2cc2dc95c6d2', 'type': 'Concept', 'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3', 'name': 'ent', 'named_type': 'Concept'}, {'handle': '80aff30094874e75028033a38ce677bb', 'type': 'Concept', 'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3', 'name': 'plant', 'named_type': 'Concept'}]}
    



```python
# This is a pattern like:
#
# AND
#     Inheritance
#         v1
#         mammal
#     Inheritance
#         v2
#         dinosaur
#     Similarity
#         v1
#         v2
#
# The expected answer is all pair of animals such that 
# one inherits from mammal, the other inherits from dinosaur 
# and they have a Similarity link between them.
#
exp1 = {
    'atom_type': 'link',
    'type': 'Inheritance',
    'targets': [
        {'atom_type': 'variable', 'name': 'v1'},
        {'atom_type': 'node', 'type': 'Concept', 'name': 'mammal'},
    ]
}
exp2 = {
    'atom_type': 'link',
    'type': 'Inheritance',
    'targets': [
        {'atom_type': 'variable', 'name': 'v2'},
        {'atom_type': 'node', 'type': 'Concept', 'name': 'dinosaur'},
    ]
}
exp3 = {
    'atom_type': 'link',
    'type': 'Similarity',
    'targets': [
        {'atom_type': 'variable', 'name': 'v1'},
        {'atom_type': 'variable', 'name': 'v2'},
    ]
}
query = [exp1, exp2, exp3] # a list of expressions mean an AND of them

for query_answer in das.query(query):
    print(query_answer.assignment)
    atom_matching_v1 = das.get_atom(query_answer.assignment.mapping['v1'])
    atom_matching_v2 = das.get_atom(query_answer.assignment.mapping['v2'])
    print("v1:", atom_matching_v1['type'], atom_matching_v1['name'])
    print("v2:", atom_matching_v2['type'], atom_matching_v2['name'])
    #rewrited_query = query_answer.subgraph
    #print(rewrited_query)
    print()
```

    [('v1', '99d18c702e813b07260baf577c60c455'), ('v2', 'd03e59654221c1e8fcda404fd5c8d6cb')]
    v1: Concept rhino
    v2: Concept triceratops
    



```python
# This is a pattern like:
#
# AND
#     Similarity
#         v1
#         v2
#     Similarity
#         v2
#         v3
#     Similarity
#         v3
#         v1
#
# The expected answer is all triplet of animals such that 
# all of them have a Similarity link with the other two.
#
exp1 = {
    'atom_type': 'link',
    'type': 'Similarity',
    'targets': [
        {'atom_type': 'variable', 'name': 'v1'},
        {'atom_type': 'variable', 'name': 'v2'},
    ]
}
exp2 = {
    'atom_type': 'link',
    'type': 'Similarity',
    'targets': [
        {'atom_type': 'variable', 'name': 'v2'},
        {'atom_type': 'variable', 'name': 'v3'},
    ]
}
exp3 = {
    'atom_type': 'link',
    'type': 'Similarity',
    'targets': [
        {'atom_type': 'variable', 'name': 'v3'},
        {'atom_type': 'variable', 'name': 'v1'},
    ]
}
query = [exp1, exp2, exp3] # a list of expressions mean an AND of them

for query_answer in das.query(query):
    atom_matching_v1 = das.get_atom(query_answer.assignment.mapping['v1'])
    atom_matching_v2 = das.get_atom(query_answer.assignment.mapping['v2'])
    atom_matching_v3 = das.get_atom(query_answer.assignment.mapping['v3'])
    print("v1:", atom_matching_v1['type'], atom_matching_v1['name'])
    print("v2:", atom_matching_v2['type'], atom_matching_v2['name'])
    print("v3:", atom_matching_v3['type'], atom_matching_v3['name'])
    print()
```

    v1: Concept monkey
    v2: Concept chimp
    v3: Concept human
    
    v1: Concept human
    v2: Concept monkey
    v3: Concept chimp
    
    v1: Concept chimp
    v2: Concept monkey
    v3: Concept human
    
    v1: Concept monkey
    v2: Concept human
    v3: Concept chimp
    
    v1: Concept human
    v2: Concept chimp
    v3: Concept monkey
    
    v1: Concept chimp
    v2: Concept human
    v3: Concept monkey
    


<a id='remotedas'></a>
## Connecting to a remote DAS

When a DAS is instantiated with a remote query engine, it will connect to a DAS Server previously populated with a knowledge base. Atoms in the remote DAS Server become available for fetching, querying and modification.

In addition to the remote DAS, an internal local DAS is also kept locally. Some of the methods in the API will look for atoms first in this local DAS before going to the remote one. Other methods can be configured to search only in one of them (remote or local) or in both. We'll explain this behavior on a case by case basis.

In our example, we'll connect to a DAS Server pre-loaded with the following MeTTa expressions:

```
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

Semantically, this is the same knowledge base we used as an example for a local DAS above. However, the mapping to nodes and links is slightly different as described in the [DAS MeTTa Parser](https://github.com/singnet/das-metta-parser) documentation. For instance, each expression, like:

```
(Similarity "ent" "human")
```

is mapped to 4 atoms. 3 nodes and 1 link as follows.

```
{
    'type': 'Expression',
    'targets': [
        {'type': 'Symbol', 'name', 'Similarity'},
        {'type': 'Symbol', 'name', '"ent"'},
        {'type': 'Symbol', 'name', '"human"'}
    ]
}
```


```python
from hyperon_das import DistributedAtomSpace

host = '45.63.85.59'
port = '8080'

remote_das = DistributedAtomSpace(query_engine='remote', host=host, port=port)
print(f"Connected to DAS Server at {host}:{port}")

print("(nodes, links) =", remote_das.count_atoms())

```

    Connected to DAS Server at 45.63.85.59:8080
    (nodes, links) = (23, 60)


Atoms can be retrieved by their properties using `get_atom()`, `get_node()`, `get_link()`, `get_incoming_links()` and `get_links()` in the same way described [here](#atomquery) for local DAS. The only difference is that the local DAS will be searched first for `get_atom()`, `get_node()`, `get_link()` before going to the remote DAS when the atom is not found locally. `get_incoming_links()` and `get_links()` will search in both, local and remote DAS, and return an iterator to the results. As we explain [here](#cache), these iterators use the cache system to sort the results and determine how atoms are fetched from the remote DAS.

`add_node()` and `add_link()` will add atoms only in the local DAS. If you add an atom that already exists in the remote DAS, the local copy is always returned by the methods above. To propagate changes to the remote DAS one needs to call `commit()`. We'll not provide examples of changes in the remote DAS here because we're using a single DAS Server to serve tests with this animals KB so if you commit changes to it everyone will be affected. So please don't use this notebook to commit changes to our test server.

`fetch()` also works in the same way (described [here](#fetch)) for a remote DAS. The only difference is that now the caller can omit the parameters for `host` and `port` which are defaulted to the connected remote DAS Server. Fetching from a different DAS Server is still possible by setting the proper values for `host` and `port`.

If you execute the cells below you'll notice a delay between each call. This is because the cache system is not in place yet so each call is issuing an actual query to the remote DAS.


```python
# Compute the handle and get the actual document for "symbol"
symbol = '"earthworm"'
symbol_handle = remote_das.get_node_handle('Symbol', symbol)
symbol_document = remote_das.get_atom(symbol_handle)
symbol_document
```




    {'handle': '665509d366ac3c2821b3b6b266f996bd',
     'type': 'Symbol',
     'composite_type_hash': '02c86eb2792f3262c21d030a87e19793',
     'name': '"earthworm"',
     'named_type': 'Symbol',
     'is_literal': True}




```python
# Get expressions like (* base_symbol *)
iterator = remote_das.get_links(link_type='Expression', link_targets=['*', symbol_handle, '*'])
for link in iterator:
    atom1 = remote_das.get_atom(link['targets'][0])
    atom2 = remote_das.get_atom(link['targets'][2])
    print(f"({atom1['name']} {symbol} {atom2['name']})")
```

    (: "earthworm" Concept)
    (Inheritance "earthworm" "animal")
    (Similarity "earthworm" "snake")



```python
# Re-adding an existing atom with a custom field
remote_das.add_node(
    {
        'type': 'Symbol',
        'name': symbol,
        'truth_value': tuple([0.1, 0.9])
    }
)
remote_das.get_node('Symbol', symbol)
```




    {'handle': '665509d366ac3c2821b3b6b266f996bd',
     'type': 'Symbol',
     'composite_type_hash': '02c86eb2792f3262c21d030a87e19793',
     'name': '"earthworm"',
     'named_type': 'Symbol',
     'truth_value': (0.1, 0.9)}




```python
# Add (to the local DAS only) a new expression mentioning the base_symbol
remote_das.add_link(
    { 
        'type': 'Expression', 
        'targets': [ 
            {'type': 'Symbol', 'name': 'Pos'}, 
            {'type': 'Symbol', 'name': symbol},
            {'type': 'Symbol', 'name': 'noun'}
        ]
    }
)
# Get expressions like (* base_symbol *) again
iterator = remote_das.get_links(link_type='Expression', link_targets=['*', symbol_handle, '*'])
for link in iterator:
    atom1 = remote_das.get_atom(link['targets'][0])
    atom2 = remote_das.get_atom(link['targets'][2])
    print(f"({atom1['name']} {symbol} {atom2['name']})")
```

    (Pos "earthworm" noun)
    (: "earthworm" Concept)
    (Inheritance "earthworm" "animal")
    (Similarity "earthworm" "snake")


The methods for traversing the hypergraph work basically in the same way as for the local DAS (this is described [here](#traversing)). Because of the way MeTTa expressions are mapped to nodes/links with only one type of node and one type of link, traversing is less intuitive from a human perspective but it still makes sense to implement algorithms. Local and remote DAS are considered by the `TraverseEngine` and the whole logic of this component is subject to the cache management rules, i.e., the cache will try to pre-fetch atoms and present query answers prioritizing more relevant atoms as the caller navigates through the atomspace hypergraph.

<a id='remotequery'></a>
### Querying a remote DAS

The Pattern Matcher in a remote DAS works basically in the same way as in a local DAS (this is described [here](#patternmatcher)). The main difference is the optional parameter `query_scope` which can be used to define the scope of the query as `local_only`, `remote_only` or `local_and_remote` (its default value is `remote_only`).


```python
query = {
    'atom_type': 'link',
    'type': 'Expression',
    'targets': [
        {'atom_type': 'variable', 'name': 'v1'},
        {'atom_type': 'node', 'type': 'Symbol', 'name': symbol},
        {'atom_type': 'variable', 'name': 'v2'}
    ]
}

# The default is to query remote_only
results = remote_das.query(query)
print("Remote only")
for query_answer in results:
    v1_atom = query_answer[1]['targets'][0]
    v2_atom = query_answer[1]['targets'][2]
    print(f"({v1_atom['name']} {symbol} {v2_atom['name']})")

results = remote_das.query(query, {'query_scope': 'local_only'})
print()
print("Local only")
for query_answer in results:
    v1_atom = query_answer.subgraph['targets'][0]
    v2_atom = query_answer.subgraph['targets'][2]
    print(f"({v1_atom['name']} {symbol} {v2_atom['name']})")

# local_and_remote is not implemented yet
#results = remote_das.query(query, {'query_scope': 'local_and_remote'})
#print("Remote + Local")
#for query_answer in results:
#    v1_atom = query_answer[1]['targets'][0]
#    v2_atom = query_answer[1]['targets'][2]
#    print(f"({v1_atom['name']} {symbol} {v2_atom['name']})")

```

    Remote only
    (Inheritance "earthworm" "animal")
    (: "earthworm" Concept)
    (Similarity "earthworm" "snake")
    
    Local only
    (Pos "earthworm" noun)


<a id='customindex'></a>
### Custom Indexes

Remote DAS allow creation of custom indexes based on custom fields in nodes or links. These indexes can be used to make subsequent custom queries.


```python
symbol_name_index = remote_das.create_field_index('node', 'name', type='Symbol')
results = remote_das.custom_query(symbol_name_index, name='"human"')
for atom in results:
    print(atom['type'], atom['name'])
```

    Symbol "human"


In this example, we're creating an index for the field `name` in nodes. `name` is supposed to be defined in every node of the knowledge base. To create an index on a field which is defined only for a certain type of node, an extra `type` parameter should be passed to define which type of nodes should enter in the index: e.g. `remote_das.create_field_index('node', 'lemma', type='Word')` would create an index for the field `lemma` on all nodes of type `Word`. This type of index works only for string or number (integer or floating point) fields.
Indexes for links can be created likewise.

<a id='dasserver'></a>
## Starting a DAS Server

A DAS Server can be set up using the [DAS Toolbox](https://github.com/singnet/das-toolbox) following these steps:

1. Setup environment variables
1. Start DB servers
1. Load MeTTa knowledge base
1. Start FaaS gateway

First, you need to install the latest version of `das-cli` in your environment. Follow the instructions in the [toolbox repo](https://github.com/singnet/das-toolbox) to make this.

Then we'll start by setting up the environment.

<span style="color:red">*THE COMMANDS BELOW WILL CREATE FILES IN YOUR FILESYSTEM*</span>.

Run the following cell.


```python
!das-cli config list
```

If it outputs something like this:

```
+----------+----------------+-----------------------+
| Service  | Name           | Value                 |
+----------+----------------+-----------------------+
| redis    | port           | 29000                 |
| redis    | container_name | das-cli-redis-29000   |
| mongodb  | port           | 28000                 |
| mongodb  | container_name | das-cli-mongodb-28000 |
| mongodb  | username       | dbadmin               |
| mongodb  | password       | dassecret             |
| loader   | container_name | das-cli-loader        |
| openfaas | container_name | das-cli-openfaas-8080 |
+----------+----------------+-----------------------+
```

It's because you already have a config file in `~/.das`. If that's the case you need to decide if you want to re-use the same port numbers or not. It's OK to have several databases in your machine. They are Docker containers listening in the given port.

If the previous `das-cli config list` command output is empty, you just need to create a new config file. You can do so by running

```
$ das-cli config set
```

In a terminal. When you have done it, run the next cell to make sure you have a config file in place.


```python
!das-cli config list
```

Containers for the DBMS servers and OpenFaas will be created listening on the given ports. Run the next cell to make sure any previously used containers are properly removed. If there are none, nothing will be done.


```python
!das-cli server stop
!das-cli faas stop
```

Now we need to start the DBMS servers.


```python
!das-cli server start
```

You can double check that the DB containers are in place listing the active docker containers.


```python
!docker ps
```

You should see containers for Redis and MongoDB listening on the ports you defined in the config file.

Now we need to load a MeTTa file. You can use your own file here or run the next cell to download the same file we used in [this section](#remotedas).


```python
!wget -o /tmp/.get.output https://raw.githubusercontent.com/singnet/das-metta-parser/master/tests/data/animals.metta && mv -f animals.metta /tmp
```

You may want to change the path in the cell below to point to another file.


```python
!das-cli metta load --path /tmp/animals.metta
```

You may call `das-cli metta load` multiple times loading different files. To clear the databases you can use `das-cli db restart`.

Once you're done loading the knowledge base, you need to start the FaaS server.


```python
!das-cli faas start
```

It's done. At this point you should be able to point one or more remote DAS to this DAS Server, as we described [here](#remotedas).
