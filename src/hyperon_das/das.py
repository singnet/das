from typing import Any, Dict, Iterator, List, Optional, Type, Union

from hyperon_das_atomdb import AtomDB, AtomDoesNotExist
from hyperon_das_atomdb.adapters import InMemoryDB, RedisMongoDB
from hyperon_das_atomdb.database import (
    AtomT,
    HandleListT,
    HandleSetT,
    HandleT,
    IncomingLinksT,
    LinkT,
    NodeT,
)
from hyperon_das_atomdb.exceptions import InvalidAtomDB

from hyperon_das.cache.cache_controller import CacheController
from hyperon_das.constants import DasType
from hyperon_das.context import Context
from hyperon_das.exceptions import (
    GetTraversalCursorException,
    InvalidDASParameters,
    InvalidQueryEngine,
)
from hyperon_das.link_filters import LinkFilter
from hyperon_das.logger import logger
from hyperon_das.query_engines.local_query_engine import LocalQueryEngine
from hyperon_das.query_engines.remote_query_engine import RemoteQueryEngine
from hyperon_das.traverse_engines import TraverseEngine
from hyperon_das.type_alias import Query
from hyperon_das.utils import QueryAnswer, get_package_version


class DistributedAtomSpace:
    backend: AtomDB

    # FIXME: B006 Do not use mutable data structures for argumnet defaults
    def __init__(self, system_parameters: Dict[str, Any] = {}, **kwargs) -> None:  # noqa: B006
        """
        Creates a new DAS object. A DAS client can run locally or locally and remote,
        connecting to remote DAS instances to query remote atoms. If there are different
        versions of the same atom in local and one of the remote DAS instances, the local
        version is returned. When running along a remote DAS, a host and port is mandatory.
        By default, local instances of the DBs are created, remote instances can be configured
        using kwargs options.

        Args:
            system_parameters (Dict[str, Any]): Sets the system parameters. Defaults to {
                'running_on_server': False, 'cache_enabled': False, 'attention_broker_hostname':
                'localhost', 'attention_broker_port': 27000}.

        Keyword Args:
            atomdb (str, optional): AtomDB type supported values are 'ram' and 'redis_mongo'.
                Defaults to 'ram'.
            query_engine (str, optional): Set the type of connection for the query engine,
                values are 'remote' or 'local'. When this arg is set to 'remote', additional
                kwargs are required as host and port to connect to the remote query engine and
                the arg mode is used to configure the read/write privileges. Defaults to 'local'.
            host (str, optional): Sets the host for the remote query engine, it's mandatory when
                the query_engine is equal to 'remote'.
            port (str, optional): Sets the port for the remote query engine, it's mandatory when
                the query_engine is equal to 'remote'.
            mode (str, optional): Set query engine's ACL privileges, only available when the
                query_engine is set to 'remote', accepts 'read-only' or 'read-write'. Defaults
                to 'read-only'.
            mongo_hostname (str, optional): MongoDB's hostname, the local or remote query engine
                can connect to a remote server or run locally. Defaults to 'localhost'.
            mongo_port (int, optional): MongoDB port, set this arg if the port is not the
                standard. Defaults to 27017.
            mongo_username (str, optional): Username used for authentication in the MongoDB
                database. Defaults to 'mongo'.
            mongo_password (str, optional): Password used for authentication in the MongoDB
                database. Defaults to 'mongo'.
            mongo_tls_ca_file (Any, optional): Full system path to the TLS certificate.
            redis_hostname (str, optional): Redis hostname, the local or remote query engine can
                connect to a remote server or run locally. Defaults to 'localhost'.
            redis_port (int, optional): Redis port, set this arg if the port is not the
                standard. Defaults to 6379.
            redis_username (str, optional): Username used for authentication in the Redis
                database, no credentials (username/password) are needed when running locally.
            redis_password (str, optional): Password used for authentication in the Redis
                database.
            redis_cluster (bool, optional): Indicates whether Redis is configured in cluster
                mode. Defaults to True.
            redis_ssl (bool, optional): Set Redis to encrypt the connection. Defaults to True.
        """
        self.system_parameters = system_parameters
        self.atomdb = kwargs.get("atomdb", "ram")
        self.query_engine_type = kwargs.get("query_engine", "local")
        self._set_default_system_parameters()
        self._set_backend(**kwargs)
        self.cache_controller = CacheController(self.system_parameters)
        self._set_query_engine(**kwargs)

    def _set_default_system_parameters(self) -> None:
        # Internals
        if not self.system_parameters.get("running_on_server"):
            self.system_parameters["running_on_server"] = False
        # Attention Broker
        if not self.system_parameters.get("cache_enabled"):
            self.system_parameters["cache_enabled"] = False
        if not self.system_parameters.get("attention_broker_hostname"):
            self.system_parameters["attention_broker_hostname"] = "localhost"
        if not self.system_parameters.get("attention_broker_port"):
            self.system_parameters["attention_broker_port"] = 27000

    def _set_backend(self, **kwargs) -> None:
        if self.atomdb == "ram":
            self.backend = InMemoryDB()
        elif self.atomdb == "redis_mongo":
            self.backend = RedisMongoDB(**kwargs)
            if self.query_engine_type != "local":
                raise InvalidDASParameters(
                    message="'redis_mongo' AtomDB requires local query engine (i.e. 'query_engine=local')"
                )
        else:
            raise InvalidAtomDB("Invalid AtomDB type. Choose either 'ram' or 'redis_mongo'")

    def _set_query_engine(self, **kwargs) -> None:
        if self.query_engine_type == "local":
            das_type = DasType.LOCAL_RAM_ONLY if self.atomdb == "ram" else DasType.LOCAL_REDIS_MONGO
            self._start_query_engine(LocalQueryEngine, das_type, **kwargs)
        elif self.query_engine_type == "remote":
            self._start_query_engine(RemoteQueryEngine, DasType.REMOTE, **kwargs)
        else:
            raise InvalidQueryEngine(
                message="Use either 'local' or 'remote'",
                details=f"query_engine={self.query_engine_type}",
            )

    def _start_query_engine(
        self,
        engine_type: Type[LocalQueryEngine | RemoteQueryEngine],
        das_type: str,
        **kwargs,
    ) -> None:
        self._das_type = das_type
        self.query_engine = engine_type(
            self.backend, self.cache_controller, self.system_parameters, **kwargs
        )
        logger().info(f"Started {das_type} DAS")

    # FIXME: B006 Do not use mutable data structures for argumnet defaults
    def _create_context(
        self,
        name: str,
        queries: List[Query] = [],  # noqa: B006
    ) -> Context:
        context_node = self.add_node(NodeT(type=Context.CONTEXT_NODE_TYPE, name=name))
        query_answer = [self.query(query, {"no_iterator": True}) for query in queries]
        context = Context(context_node, query_answer)
        self.cache_controller.add_context(context)
        return context

    @staticmethod
    def about() -> dict:
        return {
            "das": {
                "name": "hyperon-das",
                "version": get_package_version("hyperon_das"),
                "summary": "Query Engine API for Distributed AtomSpace",
            },
            "atom_db": {
                "name": "hyperon-das-atomdb",
                "version": get_package_version("hyperon_das_atomdb"),
                "summary": "Persistence layer for Distributed AtomSpace",
            },
        }

    @staticmethod
    def compute_node_handle(node_type: str, node_name: str) -> str:
        """
        Computes the handle of a node, given its type and name.

        Note that this is a static method which don't actually query the stored atomspace
        in order to compute the handle. Instead, it just runs an MD5 hashing algorithm on
        the parameters that uniquely identify nodes (i.e. type and name) This means e.g.
        that two nodes with the same type and the same name are considered to be the exact
        same entity as they will have the same handle.

        Args:
            node_type (str): Node type
            node_name (str): Node name

        Returns:
            str: Node's handle

        Examples:
            >>> das = DistributedAtomSpace()
            >>> result = das.compute_node_handle(node_type="Concept", node_name="human")
            >>> print(result)
            "af12f10f9ae2002a1607ba0b47ba8407"
        """
        return AtomDB.node_handle(node_type, node_name)

    @staticmethod
    def compute_link_handle(link_type: str, link_targets: HandleListT) -> str:
        """
        Computes the handle of a link, given its type and targets' handles.

        Note that this is a static method which don't actually query the stored atomspace
        in order to compute the handle. Instead, it just runs an MD5 hashing algorithm on
        the parameters that uniquely identify links (i.e. type and list of targets) This
        means e.g. that two links with the same type and the same targets are considered
        to be the exact same entity as they will have the same handle.

        Args:
            link_type (str): Link type.
            link_targets (HandleListT): List with the target handles.

        Returns:
           str: Link's handle.

        Examples:
            >>> das = DistributedAtomSpace()
            >>> human_handle = das.compute_node_handle(node_type="Concept", node_name="human")
            >>> monkey_handle = das.compute_node_handle(node_type="Concept", node_name="monkey")
            >>> result = das.compute_link_handle(
            ...     link_type="Similarity", targets=[human_handle, monkey_handle]
            ... )
            >>> print(result)
            "bad7472f41a0e7d601ca294eb4607c3a"

        """
        return AtomDB.link_handle(link_type, link_targets)

    def get_atom(self, handle: HandleT) -> AtomT:
        """
        Retrieve an atom given its handle.

        A handle is a MD5 hash of a node in the graph. It can be computed using
        `compute_node_handle()' or `compute_link_handle()`.

        Args:
            handle (HandleT): Atom's handle.

        Returns:
            AtomT: A dataclass instance representing an atom.

        Raises:
            AtomDoesNotExist: If the corresponding atom doesn't exist.

        Examples:
            >>> das = DistributedAtomSpace()
            >>> human_handle = das.compute_node_handle(node_type="Concept", node_name="human")
            >>> node = das.get_atom(human_handle)
            >>> print(node)
            Node(_id: 'af12f10f9ae2002a1607ba0b47ba8407', handle: 'af12f10f9ae2002a1607ba0b47ba8407',
            composite_type_hash: 'd99a604c79ce3c2e76a2f43488d5d4c3', named_type: 'Concept',
            custom_attributes: {}, name: 'human')
            >>> print(node.to_dict())
            {'_id': 'af12f10f9ae2002a1607ba0b47ba8407', 'handle': 'af12f10f9ae2002a1607ba0b47ba8407',
            'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3', 'named_type': 'Concept',
            'custom_attributes': {}, 'name': 'human'}
        """
        return self.query_engine.get_atom(handle, no_target_format=True)

    def get_atoms(self, handles: HandleListT) -> list[AtomT]:
        """
        Retrieve atoms given a list of handles.

        A handle is a MD5 hash of a node in the graph. It can be computed using
        `compute_node_handle()' or `compute_link_handle()`.

        It is preferable to call get atoms() passing a list of handles rather than
        repeatedly calling get_atom() for each handle because get_atoms() makes at
        most one remote request, if necessary.

        Args:
            handles (HandleListT): List with Atom's handles.

        Returns:
            list[AtomT]: A list of Atom instances with all atom data.

        Raises:
            AtomDoesNotExist: If some of the atoms doesn't exist.

        Examples:
            >>> das = DistributedAtomSpace()
            >>> human_handle = das.compute_node_handle(node_type="Concept", node_name="human")
            >>> animal_handle = das.compute_node_handle(node_type="Concept", node_name="monkey")
            >>> result = das.get_atoms([human_handle, animal_handle])
            >>> print(result[0])
            Node(_id: 'af12f10f9ae2002a1607ba0b47ba8407', handle: 'af12f10f9ae2002a1607ba0b47ba8407',
            composite_type_hash: 'd99a604c79ce3c2e76a2f43488d5d4c3', named_type: 'Concept',
            custom_attributes: {}, name: 'human')
            >>> print(result[1])
            Node(_id: '1cdffc6b0b89ff41d68bec237481d1e1', handle: '1cdffc6b0b89ff41d68bec237481d1e1',
            composite_type_hash: 'd99a604c79ce3c2e76a2f43488d5d4c3', named_type: 'Concept',
            custom_attributes: {}, name: 'monkey')
        """
        return self.query_engine.get_atoms(handles, no_target_format=True)

    def get_node(self, node_type: str, node_name: str) -> NodeT:
        """
        Retrieve a node given its type and name.

        Args:self.query_engine
            node_type (str): Node type
            node_name (str): Node name

        Returns:
            NodeT: A dataclass instance representing the node.

        Raises:
            AtomDoesNotExist: If the corresponding node doesn't exist.

        Examples:
            >>> das = DistributedAtomSpace()
            >>> node = das.get_node(node_type="Concept", node_name="human")
            >>> print(node)
            Node(_id: 'af12f10f9ae2002a1607ba0b47ba8407', handle: 'af12f10f9ae2002a1607ba0b47ba8407',
            composite_type_hash: 'd99a604c79ce3c2e76a2f43488d5d4c3', named_type: 'Concept',
            custom_attributes: {}, name: 'human')
            >>> print(node.to_dict())
            {'_id': 'af12f10f9ae2002a1607ba0b47ba8407', 'handle': 'af12f10f9ae2002a1607ba0b47ba8407',
            'composite_type_hash': 'd99a604c79ce3c2e76a2f43488d5d4c3', 'named_type': 'Concept',
            'custom_attributes': {}, 'name': 'human'}
        """
        node_handle = self.backend.node_handle(node_type, node_name)
        return self.get_atom(node_handle)

    def get_link(self, link_type: str, link_targets: HandleListT) -> LinkT:
        """
        Retrieve a link given its type and list of targets.
        Targets are hashes (aka handles) of other atoms, like nodes or links.

        Args:
            link_type (str): Link type
            link_targets (HandleListT): List of target handles.

        Returns:
            LinkT: A dataclass instance representing the link.

        Raises:
            AtomDoesNotExist: If the corresponding link doesn't exist.

        Examples:
            >>> das = DistributedAtomSpace()
            >>> human_handle = das.compute_node_handle("Concept", "human")
            >>> monkey_handle = das.compute_node_handle("Concept", "monkey")
            >>> result = das.get_link(
                    link_type='Similarity',
                    link_targets=[human_handle, monkey_handle],
                )
            >>> print(result)
            Link(_id: 'bad7472f41a0e7d601ca294eb4607c3a', handle: 'bad7472f41a0e7d601ca294eb4607c3a',
            composite_type_hash: 'ed73ea081d170e1d89fc950820ce1cee', named_type: 'Similarity',
            custom_attributes: {}, composite_type: ['a9dea78180588431ec64d6bc4872fdbc',
            'd99a604c79ce3c2e76a2f43488d5d4c3', 'd99a604c79ce3c2e76a2f43488d5d4c3'],
            named_type_hash: 'a9dea78180588431ec64d6bc4872fdbc',
            targets: ['af12f10f9ae2002a1607ba0b47ba8407', '1cdffc6b0b89ff41d68bec237481d1e1'],
            is_toplevel: true, targets_documents: [])
        """
        link_handle = self.backend.link_handle(link_type, link_targets)
        return self.get_atom(link_handle)

    def get_links(self, link_filter: LinkFilter) -> List[LinkT]:
        """
        Retrieves all links that match the passed filtering criteria.

        Args:
            link_filter (LinkFilter): Filtering criteria to be used to select links

        Returns:
            List[LinkT]: A list of Links.
        """
        return self.query_engine.get_links(link_filter)

    def get_link_handles(self, link_filter: LinkFilter) -> HandleSetT:
        """
        Retrieve the handle of all links that match the passed filtering criteria.

        Args:
            link_filter (LinkFilter): Filtering criteria to be used to select links

        Returns:
            HandleSetT: Link handles
        """
        return self.query_engine.get_link_handles(link_filter)

    def get_incoming_links(self, atom_handle: HandleT, **kwargs) -> IncomingLinksT:
        """
        Retrieve all links which has the passed handle as one of its targets.

        Args:
            atom_handle (HandleT): Atom's handle

        Keyword Args:
            handles_only (bool, optional): Returns a list of links handles.

        Returns:
            List[str] (when `handles_only` is True): A list of strings containing the link handles.
            List[LinkT] (when `handdles_only` is False or omitted): A list of Link instances
                holding detailed information about the incoming links.

        Examples:
            >>> das = DistributedAtomSpace()
            >>> rhino = das.compute_node_handle("Concept", "rhino")
            >>> links = das.get_incoming_links(rhino)
            >>> for link in links:
            >>>     print(link.type, link.targets)
            Similarity ['d03e59654221c1e8fcda404fd5c8d6cb', '99d18c702e813b07260baf577c60c455']
            Similarity ['99d18c702e813b07260baf577c60c455', 'd03e59654221c1e8fcda404fd5c8d6cb']
            Inheritance ['99d18c702e813b07260baf577c60c455', 'bdfe4e7a431f73386f37c6448afe5840']
        """
        return self.query_engine.get_incoming_links(atom_handle, **kwargs)

    # FIXME: B006 Do not use mutable data structures for argumnet defaults
    def count_atoms(self, parameters: Dict[str, Any] = {}) -> Dict[str, int]:  # noqa: B006
        """
        Count atoms, nodes and links in DAS.

        By default, the precise parameter is set to False returning the total number of atoms, without node and link
        counts. If the precise parameter is True it will return the total of nodes and links and atoms.

        In the case of remote DAS, count the total number of nodes and links stored locally and
        remotely. If there are more than one instance of the same atom (local and remote), it's
        counted only once.

        Args:
            parameters (Optional[Dict[str, Any]]): Dict containing the following keys: 'context' - returning the
                count of 'local', 'remote' or 'both', 'precise' - boolean if True provides an accurate but slower count,
                if False the count will be an estimate, which is faster but less precise.
                Default value for 'context' is 'both' and for 'precise' is False.
                Defaults to None.

        Returns:
            Dict[str, int]: Dict containing the keys 'node_count', 'atom_count', 'link_count'.
        """
        return self.query_engine.count_atoms(parameters)

    # FIXME: B006 Do not use mutable data structures for argumnet defaults
    def query(
        self,
        query: Query,
        parameters: Dict[str, Any] = {},  # noqa: B006
    ) -> Union[Iterator[QueryAnswer], List[QueryAnswer]]:
        """
        Perform a query on the knowledge base using a dict as input and return an
        iterator of QueryAnswer objects. Each such object carries the resulting mapping
        of variables in the query and the corresponding subgraph which is the result
        of applying such mapping to rewrite the query.

        The input dict is a link, used as a pattern to make the query.
        Variables can be used as link targets as well as nodes. Nested links are
        allowed as well.

        Args:
            query (Union[List[Dict[str, Any]], Dict[str, Any]]): A pattern described as a
                link (possibly with nested links) with nodes and variables used to query
                the knowledge base. If the query is represented as a list of dictionaries,
                it is interpreted as a conjunction (AND) of all queries within the list.
            parameters (Dict[str, Any]): query optional parameters, defaults to {}.
                Eg:
                'no_iterator' can be set to True to return a list instead of an iterator.
                'query_scope' can be set to 'remote_only' to query the remote DAS (default),
                'synchronous_update' to query remote and sync, 'local_only' to query local DAS
                or 'local_and_remote' to query both (Not available yet)

        Returns:
            Iterator[QueryAnswer]: An iterator of QueryAnswer objects, which have a field 'assignment',
                with a mapping from variables to handles and another field 'subgraph',
                with the resulting subgraph after applying 'assignment' to rewrite the query.

        Raises:
            UnexpectedQueryFormat: If query resolution lead to an invalid state

        Notes:
            - Logical connectors OR and NOT are not implemented yet.
            - If no match is found for the query, an empty list is returned.

        Examples:

            >>> das.add_link(
                    LinkT(
                        type="Expression",
                        targets=[
                            NodeT(type="Symbol", name="Test"),
                            LinkT(
                                type="Expression",
                                targets=[
                                    NodeT(type="Symbol", name="Test"),
                                    NodeT(type="Symbol", name="2"),
                                ]
                            )
                        ]
                    )
                )
            >>> query_params = {"toplevel_only": False}
            >>> q1 = {
                "atom_type": "link",
                "type": "Expression",
                "targets": [
                    {"atom_type": "variable", "name": "v1"},
                    {
                        "atom_type": "link",
                        "type": "Expression",
                        "targets": [
                            {"atom_type": "variable", "name": "v2"},
                            {"atom_type": "node", "type": "Symbol", "name": "2"},
                        ]
                    }
                ]
            }
            >>> for result in das.query(q1, query_params):
            >>>     print(result.assignment.mapping['v1'])
            >>>     print(result.assignment.mapping['v2'])
            >>>     print(result.assignment.subgraph)
            '233d9a6da7d49d4164d863569e9ab7b6'
            '963d66edfb77236054125e3eb866c8b5'
            [
                {
                    'handle': 'dbcf1c7b610a5adea335bf08f6509978',
                    'type': 'Expression',
                    'template': ['Expression', 'Symbol', ['Expression', 'Symbol', 'Symbol']],
                    'targets': [
                        {'handle': '963d66edfb77236054125e3eb866c8b5', 'type': 'Symbol', 'name': 'Test'},
                        {
                            'handle': '233d9a6da7d49d4164d863569e9ab7b6',
                            'type': 'Expression',
                            'template': ['Expression', 'Symbol', 'Symbol'],
                            'targets': [
                                {'handle': '963d66edfb77236054125e3eb866c8b5', 'type': 'Symbol', 'name': 'Test'},
                                {'handle': '9f27a331633c8bc3c49435ffabb9110e', 'type': 'Symbol', 'name': '2'}
                            ]
                        }
                    ]
                }
            ]
        """
        return self.query_engine.query(query, parameters)

    def custom_query(self, index_id: str, query: Query, **kwargs) -> Union[Iterator, List[AtomT]]:
        """
        Perform a query using a previously created custom index.

        Actual query parameters can be passed as kwargs according to the type of the previously
        created filter.

        Args:
            index_id (str): custom index id to be used in the query.
            query (Dict[str, Any]): Query dict, fields are the dict's keys and values are the search.
                It supports multiple fields.
                eg: {'name': 'human'}

        Keyword Args:
            no_iterator (bool, optional): Set False to return an iterator otherwise it will
                return a list of Dict[str, Any].
                If the query_engine is set to 'remote' it always return an iterator.
                Defaults to True.
            cursor (Any, optional): Cursor position in the iterator, starts retrieving links from redis at the cursor
                position. Defaults to 0.
            chunk_size (int, optional): Chunk size. Defaults to 1000.

        Raises:
            NotImplementedError: If called from Local DAS in RAM only.

        Returns:
            Iterator | List[AtomT]: An iterator or a list of Atom instances (Nodes or Links).

        Examples:
            >>> das.custom_query(index_id="index_123", query={"tag": "DAS"})
            >>> das.custom_query(index_id="index_123", query={"tag": "DAS"}, no_iterator=True)
        """
        if isinstance(self.query_engine, LocalQueryEngine) and isinstance(self.backend, InMemoryDB):
            raise NotImplementedError("custom_query() is not implemented for Local DAS in RAM only")

        return self.query_engine.custom_query(
            index_id, [{"field": k, "value": v} for k, v in query.items()], **kwargs
        )

    def get_atoms_by_field(self, query: Query) -> HandleListT:
        """
        Search for the atoms containing field and value, performance is improved if an index was
        previously created.

        Args:
            query (Dict[str, Any]): Query dict, fields are the dict keys and values are the search.
                It supports multiple fields.
                eg: {'name': 'human'}

        Returns:
            HandleListT: List of atoms handles
        """

        return self.query_engine.get_atoms_by_field(
            [{"field": k, "value": v} for k, v in query.items()]
        )

    def get_atoms_by_text_field(
        self,
        text_value: str,
        field: Optional[str] = None,
        text_index_id: Optional[str] = None,
    ) -> HandleListT:
        """
        Performs a text search, if a text index is previously created performance a token index search,
        otherwise will perform a regex search using binary tree and the argument 'field' is mandatory.
        Performance is improved if a 'binary_tree' or 'token_inverted_list' is previously created using
        'create_field_index' method.

        Args:
            text_value (str): Text value to search for
            field (Optional[str]): Field to check the text_value
            text_index_id (Optional[str]): Text index
        Returns:
            HandleListT: List of atom handles
        """
        return self.query_engine.get_atoms_by_text_field(text_value, field, text_index_id)

    def get_node_by_name_starting_with(self, node_type: str, startswith: str) -> HandleListT:
        """
        Performs a search in the nodes names searchin for a node starting with the 'startswith'
        value.

        Args:
            node_type (str): Node type
            startswith (str): String to search for
        Returns:
            HandleListT: List of atoms handles
        """
        return self.query_engine.get_node_by_name_starting_with(node_type, startswith)

    def commit_changes(self, **kwargs):
        """
        Commit changes (atom addition/deletion/change) to the databases or to
        the remote DAS Server, depending on the type of DAS being used.

        The behavior of this method depends on the type of DAS being used.

        1. When called in a DAS instantiated with query_engine=remote

            This is called a "Remote DAS" in the documentation. Remote DAS is
            connected to a remote DAS Server which is used to make queries,
            traversing, etc. but it also keeps a local Atomspace in RAM which is
            used as a cache. Atom changes are made initially in this local cache.
            When commit_changes() is called in this type of DAS, these changes are
            propagated to the remote DAS Server.

        2. When called in a DAS instantiated with query_engine=local and
           atomdb='ram'.

            No effect.

        3. When called in a DAS instantiated with query_engine=local and
           atomdb='redis_mongo'

            The AtomDB keeps buffers of changes which are not actually written in the
            DBs until commit_changes() is called (or until that buffers size reach a
            threshold).
        """
        self.query_engine.commit(**kwargs)

    def add_node(self, node_params: NodeT) -> NodeT:
        """
        Adds a node to DAS.

        A node is represented by a dataclass which must contain at least the attributes `type` and
        `name` mapping to strings which define the node uniquely, i.e. two nodes with the same
        `type` and `name` are considered to be the same entity.

        Args:
            node_params (NodeT): A dataclass with basic node data.
                The following attributes are mandatory:
                - `type`: Node type
                - `name`: Node name
                Optional attribute:
                - `custom_attributes`: A dictionary with custom attributes, where keys are strings
                                       and values can be [str | int | float | bool].

        Returns:
            NodeT: The complete dataclass representing the added node, including its unique handle
            and other fields used internally in DAS.

        Raises:
            AddNodeException: If `type` or `name` fields are missing or invalid somehow.

        Examples:
            >>> das = DistributedAtomSpace()
            >>> node_params = NodeT(
                    type='Reactome',
                    name='Reactome:R-HSA-164843',
                    custom_attributes={'created_at': '2021-09-01'},  # Optional
                )
            >>> das.add_node(node_params)
        """
        return self.backend.add_node(node_params)

    def add_link(self, link_params: LinkT) -> LinkT:
        """
        Adds a link to DAS.

        A link is represented by a dataclass which must contain at least the attributes `type` and
        `targets`. Where `type` should map to a string and `targets` to a list of NodeT or LinkT
        dataclasses, each of them being itself a representation of either a node or a nested link.
        `type` and `targets` define the link uniquely, i.e. two links with the same `type` and
        `targets` are considered to be the same entity.

        Args:
            link_params (LinkT): A dataclass with the link data.
                The following keys are mandatory:
                - `type`: The type of the link.
                - `targets`: A list of target elements of type NodeT or LinkT.
                Optional attribute:
                - `custom_attributes`: A dictionary with custom attributes, where keys are strings
                                       and values can be [str | int | float | bool].

        Returns:
            LinkT: The information about the added link, including its unique handle and
            other fields used internally in DAS.

        Raises:
            AddLinkException: If the `type` or `targets` fields are missing or invalid somehow.

        Examples:
            >>> das = DistributedAtomSpace()
            >>> link_params = LinkT(
                    type='Evaluation',
                    targets=[
                        NodeT(type='Predicate', name='Predicate:has_name'),
                        LinkT(
                            type='Set',
                            targets=[
                                NodeT(type='Reactome', name='Reactome:R-HSA-164843'),
                                NodeT(type='Concept', name='Concept:2-LTR circle formation'),
                            ],
                        ),
                    ],
                    custom_attributes={'confidence': 0.9},  # Optional
                )
            >>> das.add_link(link_params)
        """
        return self.backend.add_link(link_params)

    def reindex(self, pattern_index_templates: Optional[Dict[str, Dict[str, Any]]] = None):
        """
        Rebuild all indexes according to the passed specification

        Args:
            pattern_index_templates: indexes are specified by atom type in a dict mapping from atom types
                to a pattern template:

                {
                    <atom type>: <pattern template>
                }

                Pattern template is also a dict:

                {
                    "named_type": True/False
                    "selected_positions": [n1, n2, ...]
                }

                Pattern templates are applied to each link entered in the atom space in order to determine
                which entries should be created in the inverted pattern index. Entries in the inverted
                pattern index are like patterns where the link type and each of its targets may be replaced
                by wildcards. For instance, given a similarity link Similarity(handle1, handle2) it could be
                used to create any of the following entries in the inverted pattern index:

                    *(handle1, handle2)
                    Similarity(*, handle2)
                    Similarity(handle1, *)
                    Similarity(*, *)

                If we create all possibilities of index entries to all links, the pattern index size will
                grow exponentially so we limit the entries we want to create by each type of link. This is
                what a pattern template for a given link type is. For instance if we apply this pattern
                template:

                {
                    "named_type": False
                    "selected_positions": [0, 1]
                }

                to Similarity(handle1, handle2) we'll create only the following entries:

                    Similarity(*, handle2)
                    Similarity(handle1, *)
                    Similarity(*, *)

                If we apply this pattern template instead:

                {
                    "named_type": True
                    "selected_positions": [1]
                }

                We'll have:

                    *(handle1, handle2)
                    Similarity(handle1, *)
        """
        return self.query_engine.reindex(pattern_index_templates)

    def clear(self) -> None:
        """
        Delete all atoms and custom indexes.
        """
        self.backend.clear_database()
        logger().debug("The database has been cleaned.")

    def get_traversal_cursor(self, handle: str, **kwargs) -> TraverseEngine:
        """
        Create and return a [Traverse Engine](/api/Traverse Engine), an object that can be used to traverse the
        atomspace hypergraph.

        A TraverseEngine is like a cursor which points to an atom in the hypergraph and
        can be used to probe for links and neighboring atoms and then move on by
        following links. Its functioning is closely tied to the cache system in order
        to optimize the order in which atoms are presented to the caller when probing
        the neighborhood and to use cache's "atom paging" capabilities to minimize
        latency when used in remote DAS.

        Args:
            handle (str): Atom's handle

        Raises:
            GetTraversalCursorException: If passed handle is invalid, somehow (e.g. if
            the corresponding atom doesn't exist).

        Returns:
            TraverseEngine: The object that allows traversal of the hypergraph.
        """
        try:
            return TraverseEngine(handle, das=self, **kwargs)
        except AtomDoesNotExist as e:
            raise GetTraversalCursorException(
                message="Cannot start Traversal. Atom does not exist"
            ) from e

    def create_field_index(
        self,
        atom_type: str,
        fields: List[str],
        named_type: Optional[str] = None,
        composite_type: Optional[List[Any]] = None,
        index_type: Optional[str] = None,
    ) -> str:
        """
        Create a custom index on the passed field of all atoms of the passed type.

        Remote DAS allow creation of custom indexes based on custom fields in
        nodes or links. These indexes can be used to make subsequent custom queries.

        Args:
            atom_type (str): Either 'link' or 'node', if the index is to be created for
                links or nodes.
            fields (List[str]): fields where the index will be created upon
            named_type (str, optional): Only atoms of the passed type will be indexed. Defaults
                to None, meaning that atom type doesn't matter.
            composite_type (List[Any], optional): Only Atoms type of the passed composite
                type will be indexed. Defaults to None.
            index_type (Optional[str]): Type of index, values allowed are 'binary_tree' to create indexes using binary
                tree in ascending order or 'token_inverted_list' to create index for text field search the text field
                will be tokenized and every token will be an indexed. Only one token_inverted_list field is allowed.
                If set as None will create a binary tree index.
                Defaults to None.

        Raises:
            ValueError: If parameters are invalid somehow.

        Returns:
            str: The index ID. This ID should be used to make subsequent queries using this
                newly created index.

        Examples:
            >>> index_id = das.create_field_index("link", ["tag"], type="Expression")
            >>> index_id = das.create_field_index(
            ...     "link",
            ...     ["tag"],
            ...     composite_type=[
            ...         "Expression",
            ...         "Symbol",
            ...         "Symbol",
            ...         ["Expression", "Symbol", "Symbol", "Symbol"],
            ...     ],
            ... )
        """
        if named_type and composite_type:
            raise ValueError("'type' and 'composite_type' can't be specified simultaneously")

        if named_type and not isinstance(named_type, str):
            raise ValueError("'atom_type' should be str")

        if not fields:
            raise ValueError("'fields' should not be None or empty")

        return self.query_engine.create_field_index(
            atom_type,
            fields,
            named_type=named_type,
            composite_type=composite_type,
            index_type=index_type,
        )

    def fetch(
        self,
        query: Optional[Union[List[dict], dict]] = None,
        host: Optional[str] = None,
        port: Optional[int] = None,
        **kwargs,
    ) -> Union[None, List[AtomT]]:
        """
        Fetch, from a DAS Server, all atoms that match the passed query or
        all atoms in the server if None is passed as query.

        Instead of adding atoms by calling add_node() and add_link() directly,
        it's possible to fetch all or part of the contents from a DAS server using the
        method fetch(). This method doesn't create a lasting connection with the DAS
        server, it will just fetch the atoms once and close the connection so any
        subsequent changes or queries will not be propagated to the server in any way.
        After fetching the atoms, all queries will be made locally. It's possible to
        call fetch() multiple times fetching from the same DAS Server or from different
        ones.

        The input query is a link, used as a pattern to make the query.
        Variables can be used as link targets as well as nodes. Nested links are
        allowed as well.

        Args:
            query (Optional[Union[List[dict], dict]]): A pattern described as a link (possibly with
                nested links) with nodes and variables used to query the knowledge base. Defaults to None
            host (Optional[str], optional): Address to remote server. Defaults to None.
            port (Optional[int], optional): Port to remote server. Defaults to None.

        Raises:
            ValueError: If parameters ar somehow invalid.

        Returns:
            Union[None, List[AtomT]]: Returns None.
            If running on the server returns a list of Atom instances.

        Examples:
            >>> query = {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "Inheritance"},
                        {"atom_type": "variable", "name": "v1"},
                        {"atom_type": "node", "type": "Symbol", "name": '"mammal"'},
                    ],
                }
                das = DistributedAtomSpace()
                das.fetch(query, host='123.4.5.6', port=8080)
        """

        if not self.system_parameters.get("running_on_server"):
            if self._das_type != DasType.REMOTE and (not host or not port):
                raise ValueError("'host' and 'port' are mandatory parameters to local DAS")

        documents = self.query_engine.fetch(query, host, port, **kwargs)
        self.backend.bulk_insert(documents)
        return documents

    # FIXME: B006 Do not use mutable data structures for argumnet defaults
    def create_context(
        self,
        name: str,
        queries: List[Query] = [],  # noqa: B006
    ) -> Context:
        if self.query_engine_type == "local":
            return self._create_context(name, queries)
        else:
            return self.query_engine.create_context(name, queries)
