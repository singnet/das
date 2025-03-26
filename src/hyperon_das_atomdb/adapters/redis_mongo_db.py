"""
This module provides a concrete implementation of the AtomDB using Redis and MongoDB.

It includes classes and methods for managing nodes and links, handling database connections,
and performing various database operations such as creating indexes, retrieving documents,
and updating indexes. The module integrates with MongoDB for persistent storage and Redis
for caching and fast access to frequently used data.
"""

import base64
import collections
import itertools
import pickle
import re
import sys
from copy import deepcopy
from enum import Enum
from typing import Any, Iterable, Mapping, Optional, OrderedDict, TypeAlias

from pymongo import ASCENDING, MongoClient
from pymongo import errors as pymongo_errors
from pymongo.collection import Collection
from pymongo.database import Database
from redis import Redis
from redis.cluster import RedisCluster

from hyperon_das_atomdb.database import (
    WILDCARD,
    AtomDB,
    AtomT,
    FieldIndexType,
    FieldNames,
    HandleListT,
    HandleSetT,
    LinkT,
    NodeT,
)
from hyperon_das_atomdb.exceptions import (
    AtomDoesNotExist,
    ConnectionMongoDBException,
    InvalidOperationException,
)
from hyperon_das_atomdb.index import Index
from hyperon_das_atomdb.logger import logger
from hyperon_das_atomdb.utils.expression_hasher import ExpressionHasher

# pylint: disable=invalid-name
# Type aliases
DocumentT: TypeAlias = dict[str, Any]
# pylint: enable=invalid-name


def _build_redis_key(prefix: str, key: str | list[Any]) -> str:
    """
    Build a Redis key by concatenating the given prefix and key with a colon separator.

    Args:
        prefix (str): The prefix to be used in the Redis key.
        key (str): The key to be concatenated with the prefix.

    Returns:
        str: The concatenated Redis key.
    """
    return prefix + ":" + str(key)


class MongoCollectionNames(str, Enum):
    """Enum for MongoDB collection names used in the AtomDB."""

    ATOMS = "atoms"
    ATOM_TYPES = "atom_types"
    DAS_CONFIG = "das_config"


class KeyPrefix(str, Enum):
    """Enum for key prefixes used in Redis."""

    INCOMING_SET = "incoming_set"
    OUTGOING_SET = "outgoing_set"
    PATTERNS = "patterns"
    TEMPLATES = "templates"
    NAMED_ENTITIES = "names"
    CUSTOM_INDEXES = "custom_indexes"


class MongoIndexType(str, Enum):
    """Enum for MongoDB index types."""

    FIELD = "field"
    COMPOUND = "compound"
    TEXT = "text"


class _HashableDocument:
    """Class for making documents hashable."""

    def __init__(self, base: DocumentT):
        self.base = base

    def __hash__(self) -> int:
        return hash(self.base["_id"])

    def __str__(self) -> str:
        return str(self.base)


class MongoDBIndex(Index):
    """Class for managing MongoDB indexes."""

    def __init__(self, collection: Collection) -> None:
        """
        Initialize the NodeDocuments class with a MongoDB collection.

        Args:
            collection (Collection): The MongoDB collection to manage node documents.
        """
        self.collection = collection

    def create(
        self,
        atom_type: str,
        fields: list[str],
        index_type: Optional[MongoIndexType] = None,
        **kwargs,
    ) -> tuple[str, Any]:
        conditionals = {}
        if fields is None or len(fields) == 0:
            raise ValueError("Fields can not be empty or None")

        if kwargs:
            key, value = next(iter(kwargs.items()))  # only one key-value pair
            conditionals = {key: {"$eq": value}}

        index_id = f"{atom_type}_{self.generate_index_id(','.join(fields), conditionals)}" + (
            f"_{index_type.value}" if index_type else ""
        )
        idx_type: MongoIndexType = index_type or (
            MongoIndexType.COMPOUND if len(fields) > 1 else MongoIndexType.FIELD
        )
        index_props = {
            "index_type": idx_type,
            "conditionals": conditionals,
            "index_name": index_id,
            "fields": fields,
        }

        index_conditionals: dict[str, Any] = {"name": index_id}

        if conditionals:
            index_conditionals["partialFilterExpression"] = index_props["conditionals"]

        index_list: list[tuple[str, Any]]
        if idx_type == MongoIndexType.TEXT:
            index_list = [(f, "text") for f in fields]
        else:
            index_list = [(f, ASCENDING) for f in fields]  # store the index in ascending order

        if not self.index_exists(index_id):
            return (
                self.collection.create_index(index_list, **index_conditionals),
                index_props,
            )
        else:
            return index_id, index_props

    def index_exists(self, index_id: str) -> bool:
        indexes = self.collection.list_indexes()
        index_ids = [index.get("name") for index in indexes]
        return index_id in index_ids


class RedisMongoDB(AtomDB):
    """A concrete implementation using Redis and Mongo database"""

    mongo_db: Database

    def __repr__(self) -> str:
        return "<Atom database RedisMongo>"  # pragma no cover

    def __init__(self, **kwargs: Optional[dict[str, Any]]) -> None:
        """Initialize an instance of a custom class with Redis and MongoDB connections."""
        super().__init__()
        self.database_name = "das"
        self.max_pos_size_custom_index_template = 4

        self._setup_databases(**kwargs)

        self.mongo_atoms_collection = self.mongo_db.get_collection(MongoCollectionNames.ATOMS)
        self.mongo_types_collection = self.mongo_db.get_collection(MongoCollectionNames.ATOM_TYPES)
        self.all_mongo_collections = [
            (MongoCollectionNames.ATOMS, self.mongo_atoms_collection),
            (MongoCollectionNames.ATOM_TYPES, self.mongo_types_collection),
        ]
        self.pattern_index_templates: list[dict[str, Any]] | None = None
        self.pattern_templates: list | None = None
        self.mongo_das_config_collection: Collection | None = None
        if MongoCollectionNames.DAS_CONFIG not in self.mongo_db.list_collection_names():
            self.mongo_db.create_collection(MongoCollectionNames.DAS_CONFIG)

        self.mongo_das_config_collection = self.mongo_db.get_collection(
            MongoCollectionNames.DAS_CONFIG
        )
        self._setup_indexes(kwargs)
        self.wildcard_hash = ExpressionHasher.compute_hash(WILDCARD)
        self.typedef_mark_hash = ExpressionHasher.compute_hash(":")
        self.typedef_base_type_hash = ExpressionHasher.compute_hash("Type")

        self.named_type_hash: dict[str, str] = {}
        self.hash_length = len(self.typedef_base_type_hash)
        self.typedef_composite_type_hash = ExpressionHasher.composite_hash(
            [
                self.typedef_mark_hash,
                self.typedef_base_type_hash,
                self.typedef_base_type_hash,
            ]
        )
        self.mongo_bulk_insertion_buffer: dict[
            MongoCollectionNames,
            tuple[Collection[Mapping[str, Any]], set[_HashableDocument]],
        ] = {
            collection_name: (collection, set())
            for collection_name, collection in self.all_mongo_collections
        }
        self.mongo_bulk_insertion_limit = 100000
        self.max_mongo_db_document_size = 16000000
        logger().info("Database setup finished")

    def _setup_databases(self, **kwargs) -> None:
        """
        Set up connections to MongoDB and Redis databases with the provided parameters.

        Args:
            **kwargs: Additional keyword arguments for database configuration, including:
                - mongo_hostname (str)   : The hostname for the MongoDB server.
                                           Defaults to 'localhost'.
                - mongo_port (int)       : The port number for the MongoDB server.
                                           Defaults to 27017.
                - mongo_username (str)   : The username for MongoDB authentication.
                                           Defaults to 'mongo'.
                - mongo_password (str)   : The password for MongoDB authentication.
                                           Defaults to 'mongo'.
                - mongo_tls_ca_file (str): The path to the TLS CA file for MongoDB.
                                           Defaults to None.
                - redis_hostname (str)   : The hostname for the Redis server.
                                           Defaults to 'localhost'.
                - redis_port (int)       : The port number for the Redis server.
                                           Defaults to 6379.
                - redis_username (str)   : The username for Redis authentication.
                                           Defaults to None.
                - redis_password (str)   : The password for Redis authentication.
                                           Defaults to None.
                - redis_cluster (bool)   : Whether to use Redis in cluster mode.
                                           Defaults to True.
                - redis_ssl (bool)       : Whether to use SSL for Redis connection.
                                           Defaults to True.

        Raises:
            ConnectionMongoDBException: If there is an error connecting to the MongoDB server.
            ConnectionRedisException: If there is an error connecting to the Redis server.
        """
        mongo_hostname: str = kwargs.get("mongo_hostname", "localhost")
        mongo_port: int = kwargs.get("mongo_port", 27017)
        mongo_username: str = kwargs.get("mongo_username", "mongo")
        mongo_password: str = kwargs.get("mongo_password", "mongo")
        mongo_tls_ca_file: str | None = kwargs.get("mongo_tls_ca_file", None)
        redis_hostname: str = kwargs.get("redis_hostname", "localhost")
        redis_port: int = kwargs.get("redis_port", 6379)
        redis_username: str | None = kwargs.get("redis_username", None)
        redis_password: str | None = kwargs.get("redis_password", None)
        redis_cluster: bool = kwargs.get("redis_cluster", True)
        redis_ssl: bool = kwargs.get("redis_ssl", True)

        self.mongo_db = self._connection_mongo_db(
            mongo_hostname,
            mongo_port,
            mongo_username,
            mongo_password,
            mongo_tls_ca_file,
        )
        self.redis = self._connection_redis(
            redis_hostname,
            redis_port,
            redis_username,
            redis_password,
            redis_cluster,
            redis_ssl,
        )

    def _connection_mongo_db(
        self,
        mongo_hostname: str,
        mongo_port: int,
        mongo_username: str,
        mongo_password: str,
        mongo_tls_ca_file: str | None,
    ) -> Database:
        """
        Establish a connection to the MongoDB database using the provided parameters.

        This method constructs a MongoDB connection URL using the provided hostname, port,
        username, password, and optional TLS CA file. It attempts to connect to the MongoDB
        database and returns the connected database instance. If an error occurs during the
        connection, it logs the error and raises a ConnectionMongoDBException.

        Args:
            mongo_hostname (str): The hostname for the MongoDB server.
            mongo_port (int): The port number for the MongoDB server.
            mongo_username (str): The username for MongoDB authentication.
            mongo_password (str): The password for MongoDB authentication.
            mongo_tls_ca_file (str | None): The path to the TLS CA file for MongoDB. Defaults to None.

        Returns:
            Database: The connected MongoDB database instance.

        Raises:
            ConnectionMongoDBException: If there is an error creating the MongoDB client.
        """
        mongo_url = f"mongodb://{mongo_username}:{mongo_password}@{mongo_hostname}:{mongo_port}"
        if mongo_tls_ca_file:  # aws
            mongo_url += (
                f"?tls=true&tlsCAFile={mongo_tls_ca_file}"
                "&replicaSet=rs0&readPreference=secondaryPreferred&retryWrites=false"
            )
        logger().info(f"Connecting to MongoDB at {mongo_url}")
        try:
            self.mongo_db = MongoClient(mongo_url)[self.database_name]
            return self.mongo_db
        except ValueError as e:
            logger().error(f"An error occurred while creating a MongoDB client - Details: {str(e)}")
            raise ConnectionMongoDBException(
                message="error creating a MongoClient", details=str(e)
            ) from e

    @staticmethod
    def _connection_redis(
        redis_hostname: str,
        redis_port: int,
        redis_username: str | None,
        redis_password: str | None,
        redis_cluster: bool = False,
        redis_ssl: bool = False,
    ) -> Redis | RedisCluster:
        """
        Establish a connection to the Redis database using the provided parameters.

        Args:
            redis_hostname (str): The hostname for the Redis server.
            redis_port (int): The port number for the Redis server.
            redis_username (str | None): The username for Redis authentication.
            redis_password (str | None): The password for Redis authentication.
            redis_cluster (bool): Whether to use Redis in cluster mode. Defaults to False.
            redis_ssl (bool): Whether to use SSL for Redis connection. Defaults to False.

        Returns:
            Redis | RedisCluster: The connected Redis or RedisCluster instance.
        """
        redis_type = "Redis cluster" if redis_cluster else "Standalone Redis"

        message = (
            f"Connecting to {redis_type} at "
            + (
                f"{redis_username}:{len(redis_password) * '*'}@"
                if redis_username and redis_password
                else ""
            )
            + f"{redis_hostname}:{redis_port}. ssl: {redis_ssl}"
        )

        logger().info(message)

        redis_connection = {
            "host": redis_hostname,
            "port": redis_port,
            "decode_responses": True,
            "ssl": redis_ssl,
        }

        if redis_password and redis_username:
            redis_connection["password"] = redis_password
            redis_connection["username"] = redis_username

        if redis_cluster:
            return RedisCluster(**redis_connection)  # type: ignore
        else:
            return Redis(**redis_connection)  # type: ignore

    def _validate_index_templates(self, templates):
        validator = {
            "field": r"(named_type|targets\[\d+\])",
            "value": r".+",
            "positions": r"\[(\d+(,\s*\d+)*)?\]",
            "arity": r"[0-9]+",
        }
        for template in templates:
            if not isinstance(template, dict):
                raise ValueError("Templates values must be a dict")
            for k in template.keys():
                if re.search(validator[k], str(template[k])) is None:
                    raise ValueError(f"Value '{template[k]}' is not supported in '{k}'.")

    def _save_pattern_index(self, pattern_index):
        self._validate_index_templates(pattern_index)
        if self.mongo_das_config_collection is not None:
            self.mongo_das_config_collection.replace_one(
                {"_id": "pattern_index_templates"},
                {"_id": "pattern_index_templates", "templates": pattern_index},
                upsert=True,
            )
            self.pattern_templates = pattern_index

    def _load_pattern_index(self, options: dict[str, Any] | None) -> None:
        """
        This method initializes the default pattern index templates based on various
        combinations of named type and selected positions. It retrieves the custom
        pattern index templates from the collection the DAS_CONFIG collection in the
        MongoDB database if exists. Otherwise, it sets the default pattern index templates
        and save it in the MongoDB database.

        Args:
            options (dict | None): Dict containing the key 'pattern_index_templates', a list of
                templates.

        """
        default_templates = [
            {"field": "named_type", "value": "*", "positions": [0, 1, 2], "arity": 3}
        ]
        user_templates = None
        found = None
        if options is not None:
            user_templates = options.get("pattern_index_templates", None)

        if self.mongo_das_config_collection is not None:
            found = self.mongo_das_config_collection.find_one({"_id": "pattern_index_templates"})

        if found is not None:
            self.pattern_templates = found.get("templates", None) if found else None
        if self.pattern_templates is not None:
            if user_templates is not None and user_templates != self.pattern_templates:
                raise ValueError(
                    "'pattern_index_templates' value doesn't match with found on database"
                )
        else:
            self._save_pattern_index(user_templates if user_templates else default_templates)

    def _validate_template_index_and_get_position(self, template):
        if not isinstance(template["arity"], int) or template["arity"] < 0:
            raise ValueError("'arity' must be an integer greater than or equal to zero.")

        if len(template["positions"]) > self.max_pos_size_custom_index_template:
            raise ValueError(
                f"'positions' array should be less than {self.max_pos_size_custom_index_template}."
            )

        if any(pos >= template["arity"] for pos in template["positions"]):
            raise ValueError("'positions' parameter must be in range of the arity.")

        if template["field"] != "named_type" and (
            found := re.search(r"targets\[(\d+)]", template["field"])
        ):
            target_pos = int(found.group(1))
            if target_pos >= template["arity"]:
                raise ValueError("'target[]' index must be in range of arity.")
        else:
            target_pos = None

        return target_pos

    def _setup_indexes(self, options: dict[str, Any] | None) -> None:
        """
        This method reads the template list and generate the index templates.
        Additionally, it creates a field index for node names.

        Args:
            options (dict | None): Dict containing the key 'pattern_index_templates', a list of
                templates.
        """
        self._load_pattern_index(options)
        if not self.pattern_templates:
            raise ValueError("Index not loaded")

        self.pattern_index_templates = []
        for template in self.pattern_templates:
            is_named_type = template["field"] == "named_type"
            p_size = len(template["positions"])
            arity = template["arity"]
            i_size = p_size + 1
            is_wild_card = template["value"] == "*"
            target_pos = self._validate_template_index_and_get_position(template)

            values = itertools.product([True, False], repeat=i_size)

            for v in values:
                if is_wild_card and all(v) and arity == p_size:
                    continue
                if p_size == 0 and not is_wild_card:
                    continue
                if not is_wild_card and not v[0] and is_named_type:
                    continue
                t = {
                    FieldNames.TYPE_NAME: v[0]
                    if is_wild_card or not is_named_type
                    else ExpressionHasher.named_type_hash(template["value"]),
                    "target_position": target_pos,
                    "target_value": None if is_named_type else template["value"],
                    "selected_positions": [
                        template["positions"][i] for i, pos in enumerate(v[1:]) if pos
                    ],
                }
                self.pattern_index_templates.append(t)
        # NOTE creating index for name search
        self.create_field_index("node", fields=["name"])

    def _retrieve_document(self, handle: str) -> DocumentT | None:
        """
        Retrieve a document from the MongoDB collection using the given handle.

        This method searches for a document in the MongoDB collection with the specified
        handle. If the document is found and it is a link, it adds the targets to the
        document before returning it.

        Args:
            handle (str): The unique identifier for the document to be retrieved.

        Returns:
            DocumentT | None: The retrieved document if found, otherwise None.
        """
        mongo_filter = {FieldNames.ID_HASH: handle}
        if document := self.mongo_atoms_collection.find_one(mongo_filter):
            if self._is_document_link(document) and FieldNames.TARGETS not in document:
                document[FieldNames.TARGETS] = self._get_document_keys(document)
            return document
        return None

    def _build_named_type_hash_template(self, template: str | list[Any]) -> str | list[Any]:
        """
        Build a named type hash template from the given template.

        This method processes the provided template, which can be a string or a nested list of
        strings, and converts it into a hash template. If the template is a string, it retrieves
        the hash for the named type. If the template is a list, it recursively processes each
        element in the list to build the hash template.

        Args:
            template (str | list[Any]): The template to be  processed into a hash template. It
            can be a string representing a named type or a nested list of strings representing
            multiple named types.

        Returns:
            str | list[Any]: The processed hash template corresponding to the provided template.

        Raises:
            ValueError: If the template is not a string or a list of strings.
        """
        if isinstance(template, str):
            return ExpressionHasher.named_type_hash(template)
        if isinstance(template, list):
            return ExpressionHasher.composite_hash(
                [self._build_named_type_hash_template(element) for element in template]
            )
        raise ValueError("Template must be a string or an iterable of anything")

    @staticmethod
    def _get_document_keys(document: DocumentT) -> HandleListT:
        """
        Retrieve the keys from the given document.

        This method extracts the keys from the provided document. If the keys are not
        directly available, it constructs them by iterating through the document with
        a specific prefix pattern.

        Args:
            document (DocumentT): The document from which to retrieve the keys.

        Returns:
            HandleListT: A list of keys extracted from the document.
        """
        answer = document.get(FieldNames.TARGETS, document.get(FieldNames.KEYS, None))
        if isinstance(answer, list):
            return answer
        elif isinstance(answer, dict):
            return list(answer.values())

        answer = []
        index = 0
        while (key := document.get(f"{FieldNames.KEY_PREFIX}_{index}", None)) is not None:
            answer.append(key)
            index += 1
        return answer

    def _filter_non_toplevel(self, matches: HandleSetT) -> HandleSetT:
        """
        Filter out non-toplevel links from the given list of matches.

        This method iterates through the provided list of matches, retrieves the corresponding
        link documents, and checks if each link is marked as toplevel. Only the toplevel links
        are included in the returned list.

        Args:
            matches (HandleSetT): A set of link handles to be filtered.

        Returns:
            HandleSetT: A set of handles corresponding to toplevel links.
        """
        return {
            link_handle
            for link_handle in matches
            if (
                (link := self._retrieve_document(link_handle))
                and link.get(FieldNames.IS_TOPLEVEL, False)
            )
        }

    def get_node_handle(self, node_type: str, node_name: str) -> str:
        node_handle = self.node_handle(node_type, node_name)
        document = self._retrieve_document(node_handle)
        if document is not None:
            return document[FieldNames.ID_HASH]
        else:
            logger().error(
                f"Failed to retrieve node handle for {node_type}:{node_name}. "
                f"This node may not exist."
            )
            raise AtomDoesNotExist("Nonexistent atom", f"{node_type}:{node_name}")

    def get_node_name(self, node_handle: str) -> str:
        answer = self._retrieve_name(node_handle)
        if not answer:
            logger().error(
                f"Failed to retrieve node name for handle: {node_handle}. "
                "The handle may be invalid or the corresponding node does not exist."
            )
            raise ValueError(f"Invalid handle: {node_handle}")
        return answer

    def get_node_type(self, node_handle: str) -> str | None:
        document = self.get_atom(node_handle)
        return document.named_type if isinstance(document, NodeT) else None  # type: ignore

    def get_node_by_name(self, node_type: str, substring: str) -> HandleListT:
        node_type_hash = ExpressionHasher.named_type_hash(node_type)
        mongo_filter = {
            FieldNames.COMPOSITE_TYPE_HASH: node_type_hash,
            FieldNames.NODE_NAME: {"$regex": substring},
        }
        return [
            document[FieldNames.ID_HASH]
            for document in self.mongo_atoms_collection.find(mongo_filter)
        ]

    def get_atoms_by_field(self, query: list[OrderedDict[str, str]]) -> HandleListT:
        mongo_filter = collections.OrderedDict([(q["field"], q["value"]) for q in query])
        return [
            document[FieldNames.ID_HASH]
            for document in self.mongo_atoms_collection.find(mongo_filter)
        ]

    def get_atoms_by_index(
        self,
        index_id: str,
        query: list[dict[str, Any]],
        cursor: int = 0,
        chunk_size: int = 500,
    ) -> tuple[int, list[AtomT]]:
        mongo_filter = collections.OrderedDict([(q["field"], q["value"]) for q in query])
        return self._get_atoms_by_index(
            index_id, cursor=cursor, chunk_size=chunk_size, **mongo_filter
        )

    def get_atoms_by_text_field(
        self,
        text_value: str,
        field: Optional[str] = None,
        text_index_id: Optional[str] = None,
    ) -> HandleListT:
        if field is not None:
            mongo_filter = {
                field: {"$regex": text_value},
            }
        else:
            mongo_filter = {"$text": {"$search": text_value}}

        if text_index_id is not None:
            return [
                document[FieldNames.ID_HASH]
                for document in self.mongo_atoms_collection.find(mongo_filter).hint(text_index_id)
            ]

        return [
            document[FieldNames.ID_HASH]
            for document in self.mongo_atoms_collection.find(mongo_filter)
        ]

    def get_node_by_name_starting_with(self, node_type: str, startswith: str):
        node_type_hash = ExpressionHasher.named_type_hash(node_type)
        mongo_filter = {
            FieldNames.COMPOSITE_TYPE_HASH: node_type_hash,
            FieldNames.NODE_NAME: {"$regex": f"^{startswith}"},
        }
        # NOTE check projection to return only required fields, less data, but is faster?
        # ex: self.mongo_atoms_collection.find(mongo_filter, projection={FieldNames.ID_HASH: 1}
        return [
            document[FieldNames.ID_HASH]
            for document in self.mongo_atoms_collection.find(mongo_filter)
        ]

    def get_all_nodes_handles(self, node_type: str) -> list[str]:
        return [
            document[FieldNames.ID_HASH]
            for document in self.mongo_atoms_collection.find({FieldNames.TYPE_NAME: node_type})
        ]

    def get_all_nodes_names(self, node_type: str) -> list[str]:
        return [
            document[FieldNames.NODE_NAME]
            for document in self.mongo_atoms_collection.find({FieldNames.TYPE_NAME: node_type})
        ]

    def get_all_links(self, link_type: str, **kwargs) -> HandleSetT:
        pymongo_cursor = self.mongo_atoms_collection.find({FieldNames.TYPE_NAME: link_type})
        return {document[FieldNames.ID_HASH] for document in pymongo_cursor}

    def get_link_handle(self, link_type: str, target_handles: HandleListT) -> str:
        link_handle = self.link_handle(link_type, target_handles)
        document = self._retrieve_document(link_handle)
        if document is not None:
            return document[FieldNames.ID_HASH]
        else:
            logger().error(
                f"Failed to retrieve link handle for {link_type}:{target_handles}. "
                "This link may not exist."
            )
            raise AtomDoesNotExist("Nonexistent atom", f"{link_type}:{target_handles}")

    def get_link_targets(self, link_handle: str) -> HandleListT:
        answer = self._retrieve_outgoing_set(link_handle)
        if not answer:
            logger().error(
                f"Failed to retrieve link targets for handle: {link_handle}. "
                "The handle may be invalid or the corresponding link does not exist."
            )
            raise ValueError(f"Invalid handle: {link_handle}")
        return answer

    def get_matched_links(
        self, link_type: str, target_handles: HandleListT, **kwargs
    ) -> HandleSetT:
        if link_type != WILDCARD and WILDCARD not in target_handles:
            try:
                link_handle = self.get_link_handle(link_type, target_handles)
                return {link_handle}
            except AtomDoesNotExist:
                return set()

        link_type_hash = (
            WILDCARD if link_type == WILDCARD else ExpressionHasher.named_type_hash(link_type)
        )

        pattern_hash = ExpressionHasher.composite_hash([link_type_hash, *target_handles])
        patterns_matched = self._retrieve_hash_targets_value(KeyPrefix.PATTERNS, pattern_hash)
        if kwargs.get("toplevel_only", False):
            return self._filter_non_toplevel(patterns_matched)
        else:
            return patterns_matched

    def get_incoming_links_handles(self, atom_handle: str, **kwargs) -> HandleListT:
        links = self._retrieve_incoming_set(atom_handle, **kwargs)
        return list(links)

    def get_incoming_links_atoms(self, atom_handle: str, **kwargs) -> list[AtomT]:
        links = self._retrieve_incoming_set(atom_handle, **kwargs)
        return [self.get_atom(handle, **kwargs) for handle in links]

    def get_matched_type_template(self, template: list[Any], **kwargs) -> HandleSetT:
        try:
            template_hash = self._build_named_type_hash_template(template)
            templates_matched = self._retrieve_hash_targets_value(
                KeyPrefix.TEMPLATES, template_hash
            )
            if kwargs.get("toplevel_only", False):
                return self._filter_non_toplevel(templates_matched)
            else:
                return templates_matched
        except Exception as exception:
            logger().error(f"Failed to get matched type template - Details: {str(exception)}")
            raise ValueError(str(exception)) from exception

    def get_matched_type(self, link_type: str, **kwargs) -> HandleSetT:
        named_type_hash = ExpressionHasher.named_type_hash(link_type)
        templates_matched = self._retrieve_hash_targets_value(KeyPrefix.TEMPLATES, named_type_hash)
        if kwargs.get("toplevel_only", False):
            return self._filter_non_toplevel(templates_matched)
        else:
            return templates_matched

    def get_link_type(self, link_handle: str) -> str | None:
        document = self.get_atom(link_handle)
        return document.named_type if isinstance(document, LinkT) else None  # type: ignore

    def _build_atom_from_dict(self, document: DocumentT) -> AtomT:
        """
        Builds an Atom object from a dictionary.

        Args:
            document (DocumentT): The dictionary representing the atom.

        Returns:
            AtomT: The constructed Atom object.
        """
        if "targets" in document:
            link = LinkT(
                handle=document[FieldNames.ID_HASH],
                _id=document[FieldNames.ID_HASH],
                named_type=document[FieldNames.TYPE_NAME],
                targets=document[FieldNames.TARGETS],
                composite_type=document[FieldNames.COMPOSITE_TYPE],
                is_toplevel=document.get(FieldNames.IS_TOPLEVEL, True),
                named_type_hash=document[FieldNames.TYPE_NAME_HASH],
                composite_type_hash=document[FieldNames.COMPOSITE_TYPE_HASH],
                custom_attributes=document.get(FieldNames.CUSTOM_ATTRIBUTES, dict()),
            )
            return link
        elif "name" in document:
            node = NodeT(
                handle=document[FieldNames.ID_HASH],
                _id=document[FieldNames.ID_HASH],
                named_type=document[FieldNames.TYPE_NAME],
                name=document[FieldNames.NODE_NAME],
                composite_type_hash=document[FieldNames.COMPOSITE_TYPE_HASH],
                custom_attributes=document.get(FieldNames.CUSTOM_ATTRIBUTES, dict()),
            )
            return node
        else:
            raise ValueError("Invalid atom type")

    def _get_atom(self, handle: str) -> AtomT | None:
        document = self._retrieve_document(handle)
        if not document:
            return None
        return self._build_atom_from_dict(document)

    def get_atom_type(self, handle: str) -> str | None:
        atom = self._retrieve_document(handle)
        if atom is None:
            return None
        return atom[FieldNames.TYPE_NAME]

    def count_atoms(self, parameters: dict[str, Any] | None = None) -> dict[str, int]:
        atom_count = self.mongo_atoms_collection.estimated_document_count()
        return_count = {"atom_count": atom_count}
        if parameters and parameters.get("precise"):
            nodes_count = self.mongo_atoms_collection.count_documents(
                {FieldNames.COMPOSITE_TYPE: {"$exists": False}}
            )
            links_count = self.mongo_atoms_collection.count_documents(
                {FieldNames.COMPOSITE_TYPE: {"$exists": True}}
            )
            return_count["node_count"] = nodes_count
            return_count["link_count"] = links_count
            return return_count

        return return_count

    def clear_database(self) -> None:
        """
        from the connected MongoDB and Redis databases.

        This method drops all collections in the MongoDB database and flushes
        all data from the Redis cache, effectively wiping the databases clean.
        """
        mongo_collections = self.mongo_db.list_collection_names()

        for collection in mongo_collections:
            self.mongo_db[collection].drop()

        self.redis.flushall()

    def commit(self, **kwargs) -> None:
        id_tag = FieldNames.ID_HASH

        if kwargs.get("buffer"):
            try:
                for document in kwargs["buffer"]:
                    self.mongo_atoms_collection.replace_one(
                        {id_tag: document[id_tag]}, document, upsert=True
                    )
                    self._update_atom_indexes([document])

            except Exception as e:
                logger().error(f"Failed to commit buffer - Details: {str(e)}")
                raise e
        else:
            for key, (collection, buffer) in self.mongo_bulk_insertion_buffer.items():
                if buffer:
                    if key == MongoCollectionNames.ATOM_TYPES:
                        msg = "Failed to commit Atom Types. This operation is not allowed"
                        logger().error(msg)
                        raise InvalidOperationException(msg)

                    for hashtable in buffer:
                        document = hashtable.base
                        collection.replace_one({id_tag: document[id_tag]}, document, upsert=True)
                        self._update_atom_indexes([document])

                buffer.clear()

    def add_node(self, node_params: NodeT) -> NodeT | None:
        node: NodeT = self._build_node(node_params)
        if sys.getsizeof(node_params.name) < self.max_mongo_db_document_size:
            _, buffer = self.mongo_bulk_insertion_buffer[MongoCollectionNames.ATOMS]
            buffer.add(_HashableDocument(node.to_dict()))
            if len(buffer) >= self.mongo_bulk_insertion_limit:
                self.commit()
            return node
        else:
            logger().warning(f"Discarding atom whose name is too large: {node.name}")
            return None

    def _build_link(self, link_params: LinkT, toplevel: bool = True) -> LinkT | None:
        # This is necessary because `_build_link` in the parent class (implemented in C++)
        # calls back to `add_link`. Without this, `nanobind` is not able to find `add_link`
        # implementation in the child class, and raises a `RuntimeError` with the message that
        # it is trying to call an abstract method (virtual pure).
        return super()._build_link(link_params, toplevel)

    def add_link(self, link_params: LinkT, toplevel: bool = True) -> LinkT | None:
        link: LinkT | None = self._build_link(link_params, toplevel)
        if link is None:
            return None
        _, buffer = self.mongo_bulk_insertion_buffer[MongoCollectionNames.ATOMS]
        buffer.add(_HashableDocument(link.to_dict()))
        if len(buffer) >= self.mongo_bulk_insertion_limit:
            self.commit()
        return link

    def _get_and_delete_links_by_handles(self, handles: HandleListT) -> list[DocumentT]:
        documents = []
        for handle in handles:
            if document := self.mongo_atoms_collection.find_one_and_delete(
                {FieldNames.ID_HASH: handle}
            ):
                documents.append(document)
        return documents

    @staticmethod
    def _apply_index_template(
        template: dict[str, Any], named_type: str, targets: HandleListT, arity: int
    ) -> str | None:
        """
        Apply the index template to generate a Redis key.

        This method constructs a Redis key by applying the provided index template. The key
        is built using the named type and the targets, with specific positions selected based
        on the template. The key is then hashed to create a unique identifier.

        Args:
            template (dict[str, Any]): The index template containing type name and selected
                positions.
            named_type (str): The named type to be included in the key.
            targets (HandleListT): The list of target handles to be included in the key.
            arity (int): The arity of the link, indicating the number of targets.

        Returns:
            str: The generated Redis key after applying the index template.
        """
        key = None
        if isinstance(template[FieldNames.TYPE_NAME], bool):
            key = [WILDCARD] if template[FieldNames.TYPE_NAME] else [named_type]
        else:
            if named_type == template[FieldNames.TYPE_NAME]:
                key = [template[FieldNames.TYPE_NAME]]

        if key is None:
            return None

        target_selected_pos = template["selected_positions"]
        if template["target_position"] is not None and len(targets) > template["target_position"]:
            if targets[template["target_position"]] == template["target_value"]:
                for cursor in range(arity):
                    key.append(WILDCARD if cursor in target_selected_pos else targets[cursor])
            else:
                return None
        else:
            for cursor in range(arity):
                key.append(WILDCARD if cursor in target_selected_pos else targets[cursor])
        return _build_redis_key(KeyPrefix.PATTERNS, ExpressionHasher.composite_hash(key))

    def _retrieve_incoming_set(self, handle: str, **kwargs) -> HandleSetT:
        """
        Retrieve the incoming set for the given handle from Redis.

        This method constructs a Redis key using the provided handle and retrieves the members
        of the incoming set associated with that key.

        Args:
            handle (str): The unique identifier for the atom whose incoming set is to be retrieved.
            **kwargs: Additional keyword arguments.

        Returns:
            HandleSetT: Set of members for the given key
        """
        key = _build_redis_key(KeyPrefix.INCOMING_SET, handle)
        return self._get_redis_members(key)

    def _delete_smember_incoming_set(self, handle: str, smember: str) -> None:
        """
        Remove a specific member from the incoming set of the given handle in Redis.

        This method constructs a Redis key using the provided handle and removes the specified
        member from the incoming set associated with that key.

        Args:
            handle (str): The unique identifier for the atom whose incoming set member is to be
                removed.
            smember (str): The member to be removed from the incoming set.

        Returns:
            None
        """
        key = _build_redis_key(KeyPrefix.INCOMING_SET, handle)
        self.redis.srem(key, smember)

    def _retrieve_and_delete_incoming_set(self, handle: str) -> HandleListT:
        """
        Retrieve and delete the incoming set for the given handle from Redis.

        This method constructs a Redis key using the provided handle, retrieves all members
        of the incoming set associated with that key, and then deletes the key from Redis.

        Args:
            handle (str): The unique identifier for the atom whose incoming set is to be
                retrieved and deleted.

        Returns:
            HandleListT: A list of members in the incoming set before deletion.
        """
        key = _build_redis_key(KeyPrefix.INCOMING_SET, handle)
        data: HandleListT = list(self.redis.smembers(key))  # type: ignore
        self.redis.delete(key)
        return data

    def _retrieve_outgoing_set(self, handle: str, delete: bool = False) -> HandleListT:
        """
        Retrieve the outgoing set for the given handle from Redis.

        This method constructs a Redis key using the provided handle and retrieves the members
        of the outgoing set associated with that key. If the delete flag is set to True, the
        key is deleted from Redis after retrieving the members.

        Args:
            handle (str): The unique identifier for the atom whose outgoing set is to be retrieved.
            delete (bool): Whether to delete the key from Redis after retrieving the members.
                Defaults to False.

        Returns:
            HandleListT: A list of members in the outgoing set.
        """
        key = _build_redis_key(KeyPrefix.OUTGOING_SET, handle)
        value: str
        if delete:
            value = self.redis.getdel(key)  # type: ignore
        else:
            value = self.redis.get(key)  # type: ignore
        if value is None:
            return []
        arity = len(value) // self.hash_length
        return [
            value[(offset * self.hash_length) : ((offset + 1) * self.hash_length)]  # noqa: E203
            for offset in range(arity)
        ]

    def _retrieve_name(self, handle: str) -> str | None:
        """
        Retrieve the name associated with the given handle from Redis.

        This method constructs a Redis key using the provided handle and retrieves the name
        associated with that key. If the name is not found, it returns None.

        Args:
            handle (str): The unique identifier for the atom whose name is to be retrieved.

        Returns:
            str | None: The name associated with the given handle if found, otherwise None.
        """
        key = _build_redis_key(KeyPrefix.NAMED_ENTITIES, handle)
        name: str = self.redis.get(key)  # type: ignore
        if name:
            return name
        else:
            return None

    def _retrieve_hash_targets_value(self, key_prefix: str, handle: str | list[Any]) -> HandleSetT:
        """
        Retrieve the hash targets value for the given handle from Redis.

        This method constructs a Redis key using the provided key prefix and handle, and retrieves
        the members associated with that key.
        If the members are not found, it returns an empty list.

        Args:
            key_prefix (str): The prefix to be used in the Redis key.
            handle (str): The unique identifier for the atom whose hash targets value is to be
                retrieved.

        Returns:
            HandleSetT: Set of members in the hash targets value.
        """
        key = _build_redis_key(key_prefix, handle)
        return self._get_redis_members(key)

    def _delete_smember_template(self, handle: str, smember: str) -> None:
        """
        Remove a specific member from the template set of the given handle in Redis.

        This method constructs a Redis key using the provided handle and removes the specified
        member from the template set associated with that key.

        Args:
            handle (str): The unique identifier for the atom whose template member is to be
                removed.
            smember (str): The member to be removed from the template set.
        """
        key = _build_redis_key(KeyPrefix.TEMPLATES, handle)
        self.redis.srem(key, smember)

    def _retrieve_custom_index(self, index_id: str) -> dict[str, Any] | None:
        """
        Retrieve a custom index from Redis using the given index ID.

        This method constructs a Redis key using the provided index ID and attempts to retrieve
        the custom index associated with that key. The custom index is expected to be stored as
        a base64-encoded, pickled dictionary. If the custom index is not found or if there is an
        error during retrieval, appropriate logging is performed and the method returns None.

        Args:
            index_id (str): The unique identifier for the custom index to be retrieved.

        Returns:
            dict[str, Any] | None: The retrieved custom index as a dictionary if found, otherwise
                None.

        Raises:
            ConnectionError: If there is an error connecting to Redis.
            Exception: If there is an unexpected error during retrieval.
        """
        try:
            key = _build_redis_key(KeyPrefix.CUSTOM_INDEXES, index_id)
            custom_index_str: str | None = self.redis.get(key)  # type: ignore

            if custom_index_str is None:
                logger().info(f"Custom index with ID {index_id} not found in Redis")
                return None

            custom_index_bytes = base64.b64decode(custom_index_str)
            custom_index: dict[str, Any] = pickle.loads(custom_index_bytes)

            if not isinstance(custom_index, dict):
                logger().error(f"Custom index with ID {index_id} is not a dictionary")
                raise ValueError("Custom index is not a dictionary")

            return custom_index
        except ConnectionError as e:
            logger().error(f"Error connecting to Redis: {e}")
            raise e
        except Exception as e:
            logger().error(f"Unexpected error retrieving custom index with ID {index_id}: {e}")
            raise e

    def _get_redis_members(self, key: str) -> HandleSetT:
        """
        Retrieve members from a Redis set.

        Args:
            key (str): The key of the set in Redis.

        Returns:
            HandleSetT: Set of members retrieved from Redis.
        """
        return set(self.redis.smembers(key))  # type: ignore

    def _update_atom_indexes(self, documents: Iterable[DocumentT], **kwargs) -> None:
        """
        Update the indexes for the given documents in the database.

        This method iterates through the provided documents and updates the corresponding
        indexes. If a document is identified as a link, it updates the link index; otherwise,
        it updates the node index.

        Args:
            documents (Iterable[DocumentT): An iterable of documents to be indexed.
            **kwargs: Additional keyword arguments for index updates.
        """
        for document in documents:
            if self._is_document_link(document):
                self._update_link_index(document, **kwargs)
            else:
                self._update_node_index(document, **kwargs)

    def _update_node_index(self, document: DocumentT, **kwargs) -> None:
        """
        Update the index for the given node document in the database.

        This method updates the Redis index for the provided node document. It constructs a Redis
        key using the document's handle and updates the node name in the Redis cache. If the
        `delete_atom` flag is set to True, it deletes the Redis key and any associated incoming
        links for the node.

        Args:
            document (DocumentT): The node document to be indexed.
            **kwargs: Additional keyword arguments for index updates. Supports `delete_atom` to
                indicate whether the node should be deleted from the index.
        """
        handle = document[FieldNames.ID_HASH]
        node_name = document[FieldNames.NODE_NAME]
        key = _build_redis_key(KeyPrefix.NAMED_ENTITIES, handle)
        if kwargs.get("delete_atom", False):
            self.redis.delete(key)
            if links_handle := self._retrieve_and_delete_incoming_set(handle):
                documents = self._get_and_delete_links_by_handles(links_handle)
                for _document in documents:
                    self._update_link_index(_document, delete_atom=True)
        else:
            self.redis.set(key, node_name)

    def _update_link_index(self, document: DocumentT, **kwargs) -> None:
        """
        Update the index for the given link document in the database.

        This method updates the Redis index for the provided link document. It constructs a Redis
        key using the document's handle and updates the link targets in the Redis cache. If the
        `delete_atom` flag is set to True, it deletes the Redis key and any associated incoming
        links for the link.

        Args:
            document (DocumentT): The link document to be indexed.
            **kwargs: Additional keyword arguments for index updates. Supports `delete_atom` to
                indicate whether the link should be deleted from the index.
        """
        handle: str = document[FieldNames.ID_HASH]
        targets: HandleListT = self._get_document_keys(document)
        targets_str: str = "".join(targets)
        arity: int = len(targets)
        named_type_hash: str = document[FieldNames.TYPE_NAME_HASH]

        index_templates = self.pattern_index_templates or []

        if kwargs.get("delete_atom", False):
            links_handle = self._retrieve_and_delete_incoming_set(handle)

            if links_handle:
                docs = self._get_and_delete_links_by_handles(links_handle)
                for doc in docs:
                    self._update_link_index(doc, delete_atom=True)

            outgoing_atoms = self._retrieve_outgoing_set(handle, delete=True)

            for atom_handle in outgoing_atoms:
                self._delete_smember_incoming_set(atom_handle, handle)

            for type_hash in [
                FieldNames.COMPOSITE_TYPE_HASH,
                FieldNames.TYPE_NAME_HASH,
            ]:
                self._delete_smember_template(document[type_hash], handle)

            for template in index_templates:
                key = self._apply_index_template(template, named_type_hash, targets, arity)
                if key:
                    self.redis.srem(key, handle)
        else:
            incoming_buffer: dict[str, HandleListT] = {}
            key = _build_redis_key(KeyPrefix.OUTGOING_SET, handle)
            self.redis.set(key, targets_str)

            for target in targets:
                buffer = incoming_buffer.get(target, None)
                if buffer is None:
                    buffer = []
                    incoming_buffer[target] = buffer
                buffer.append(handle)

            for type_hash in [
                FieldNames.COMPOSITE_TYPE_HASH,
                FieldNames.TYPE_NAME_HASH,
            ]:
                key = _build_redis_key(KeyPrefix.TEMPLATES, document[type_hash])
                self.redis.sadd(key, handle)

            for template in index_templates:
                key = self._apply_index_template(template, named_type_hash, targets, arity)
                if key:
                    self.redis.sadd(key, handle)

            for handle in incoming_buffer:
                key = _build_redis_key(KeyPrefix.INCOMING_SET, handle)
                self.redis.sadd(key, *incoming_buffer[handle])

    @staticmethod
    def _is_document_link(document: DocumentT) -> bool:
        """
        Determine if the given document is a link.

        This method checks if the provided document contains the `COMPOSITE_TYPE` field, which
        indicates that the document is a link.

        Args:
            document (DocumentT): The document to be checked.

        Returns:
            bool: True if the document is a link, False otherwise.
        """
        return FieldNames.COMPOSITE_TYPE in document

    @staticmethod
    def _calculate_composite_type_hash(composite_type: list[Any]) -> str:
        """
        Calculate the composite type hash for the given composite type.

        This method computes the hash for the provided composite type by iterating through
        the elements of the composite type. If an element is a list, it recursively calculates
        the hash for the nested list. The final hash is generated using the ExpressionHasher.

        Args:
            composite_type (list[Any]): The composite type for which the hash is to be calculated.

        Returns:
            str: The calculated composite type hash.
        """

        def calculate_composite_type_hashes(_composite_type: list[Any]) -> HandleListT:
            return [
                (
                    ExpressionHasher.composite_hash(calculate_composite_type_hashes(t))
                    if isinstance(t, list)
                    else ExpressionHasher.named_type_hash(t)
                )
                for t in _composite_type
            ]

        composite_type_hashes_list = calculate_composite_type_hashes(composite_type)
        return ExpressionHasher.composite_hash(composite_type_hashes_list)

    def _retrieve_documents_by_index(
        self, collection: Collection, index_id: str, **kwargs
    ) -> tuple[int, list[DocumentT]]:
        """
        Retrieve documents from the specified MongoDB collection using the given index.

        This method retrieves documents from the provided MongoDB collection by utilizing the
        specified index. It supports additional keyword arguments for cursor-based pagination
        and chunk size.

        Args:
            collection (Collection): The MongoDB collection from which documents are to be retrieved.
            index_id (str): The identifier of the index to be used for retrieval.
            **kwargs: Additional keyword arguments for retrieval.
                - cursor (int, optional): The cursor position for pagination.
                - chunk_size (int, optional): The number of documents to retrieve per chunk.

        Returns:
            tuple[int, list[DocumentT]]: A tuple containing the cursor position and a list of
            retrieved documents.

        Raises:
            ValueError: If the specified index does not exist in the collection.
        """
        if MongoDBIndex(collection).index_exists(index_id):
            cursor: int | None = kwargs.pop("cursor", None)
            chunk_size = kwargs.pop("chunk_size", 500)

            try:
                # Fallback to previous version
                conditionals = self._retrieve_custom_index(index_id)
                if isinstance(conditionals, dict) and (
                    (c := conditionals.get("conditionals")) or c == {}
                ):
                    conditionals = c
                if conditionals:
                    kwargs.update(conditionals)
            except Exception as e:
                raise e

            # Using the hint() method is an additional measure to ensure its use
            pymongo_cursor = collection.find(kwargs).hint(index_id)

            if cursor is not None:
                pymongo_cursor.skip(cursor).limit(chunk_size)

                documents = list(pymongo_cursor)

                if not documents:
                    return 0, []

                if len(documents) < chunk_size:
                    return 0, documents
                else:
                    return cursor + chunk_size, documents

            return 0, list(pymongo_cursor)
        else:
            raise ValueError(f"Index '{index_id}' does not exist in collection '{collection}'")

    def reindex(self, pattern_index_templates: dict[str, list[DocumentT]] | None = None) -> None:
        if isinstance(pattern_index_templates, list):
            self._save_pattern_index(deepcopy(pattern_index_templates))
            self._setup_indexes({"pattern_index_templates": pattern_index_templates})
        self.redis.flushall()
        self._update_atom_indexes(self.mongo_atoms_collection.find({}))

    def delete_atom(self, handle: str, **kwargs) -> None:
        self.commit()

        mongo_filter: dict[str, str] = {FieldNames.ID_HASH: handle}

        document: DocumentT | None = self.mongo_atoms_collection.find_one_and_delete(mongo_filter)

        if not document:
            logger().error(
                f"Failed to delete atom for handle: {handle}. "
                f"This atom may not exist. - Details: {kwargs}"
            )
            raise AtomDoesNotExist(
                message="Nonexistent atom",
                details=f"handle: {handle}",
            )
        self._update_atom_indexes([document], delete_atom=True)

    def create_field_index(
        self,
        atom_type: str,
        fields: list[str],
        named_type: Optional[str] = None,
        composite_type: Optional[list[Any]] = None,
        index_type: Optional[FieldIndexType] = None,
    ) -> str:
        if named_type and composite_type:
            raise ValueError("Both named_type and composite_type cannot be specified")

        if fields is None or len(fields) == 0:
            raise ValueError("Fields can not be empty or None")

        kwargs: dict[str, Any] = {}

        if named_type:
            kwargs = {FieldNames.TYPE_NAME: named_type}
        elif composite_type:
            kwargs = {
                FieldNames.COMPOSITE_TYPE_HASH: self._calculate_composite_type_hash(composite_type)
            }

        collection = self.mongo_atoms_collection

        index_id = ""

        mongo_index_type = (
            MongoIndexType.TEXT if index_type == FieldIndexType.TOKEN_INVERTED_LIST else None
        )

        exc: Exception | None = None
        try:
            index_id, index_props = MongoDBIndex(collection).create(
                atom_type, fields, index_type=mongo_index_type, **kwargs
            )
            serialized_index_props = pickle.dumps(index_props)
            serialized_index_props_str = base64.b64encode(serialized_index_props).decode("utf-8")
            self.redis.set(
                _build_redis_key(KeyPrefix.CUSTOM_INDEXES, index_id),
                serialized_index_props_str,
            )
        except pymongo_errors.OperationFailure as e:
            exc = e
            logger().error(f"Error creating index in collection '{collection}': {str(e)}")
        except Exception as e:  # pylint: disable=broad-except
            exc = e
            logger().error(f"Error: {str(e)}")
        finally:
            if not index_id:
                # B012: `return` inside `finally` blocks cause exceptions to be silenced
                return (  # noqa: B012
                    f"Index creation failed, Details: {str(exc)}"
                    if exc
                    else "Index creation failed"
                )

        return index_id

    def _get_atoms_by_index(self, index_id: str, **kwargs) -> tuple[int, list[AtomT]]:
        """
        Retrieve atoms from the MongoDB collection using the specified index.

        This method retrieves atoms from the MongoDB collection by utilizing the specified
        index. It supports additional keyword arguments for cursor-based pagination and
        chunk size.

        Args:
            index_id (str): The identifier of the index to be used for retrieval.
            **kwargs: Additional keyword arguments for retrieval.
                - cursor (int, optional): The cursor position for pagination.
                - chunk_size (int, optional): The number of documents to retrieve per chunk.

        Returns:
            tuple[int, list[AtomT]]: A tuple containing the cursor position and a list of
            retrieved atoms.

        Raises:
            Exception: If there is an error retrieving atoms by index.
        """
        try:
            cursor, documents = self._retrieve_documents_by_index(
                self.mongo_atoms_collection, index_id, **kwargs
            )
            return cursor, [self.get_atom(document[FieldNames.ID_HASH]) for document in documents]
        except Exception as e:
            logger().error(f"Error retrieving atoms by index: {str(e)}")
            raise e

    def retrieve_all_atoms(self) -> list[AtomT]:
        try:
            all_atoms: list[AtomT] = []
            document: DocumentT = {}
            for document in self.mongo_atoms_collection.find():
                if self._is_document_link(document) and FieldNames.TARGETS not in document:
                    document[FieldNames.TARGETS] = self._get_document_keys(document)
                atom = self._build_atom_from_dict(document)
                all_atoms.append(atom)
            return all_atoms
        except Exception as e:
            logger().error(f"Error retrieving all atoms: {type(e)}: {str(e)}, {document=}")
            raise e

    def bulk_insert(self, documents: list[AtomT]) -> None:
        """
        Insert multiple documents into the MongoDB collection and update indexes.

        This method performs a bulk insert of the provided documents into the MongoDB collection.
        It replaces existing documents with the same ID and updates the corresponding indexes.
        Additional keyword arguments can be used to customize the insertion behavior.

        Args:
            documents (list[AtomT]): A list of atoms to be inserted into the collection.

        Raises:
            pymongo.errors.BulkWriteError: If there is an error during the bulk write operation.
            Exception: If there is an unexpected error during the insertion process.
        """
        try:
            _id = FieldNames.ID_HASH
            docs: list[DocumentT] = [d.to_dict() for d in documents]
            for document in docs:
                self.mongo_atoms_collection.replace_one({_id: document[_id]}, document, upsert=True)
            self._update_atom_indexes(docs)
        except Exception as e:  # pylint: disable=broad-except
            logger().error(f"Error bulk inserting documents: {str(e)}")
