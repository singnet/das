from typing import TypeAlias

from hyperon_das_atomdb_cpp.constants import (
    TYPE_HASH,
    TYPEDEF_MARK_HASH,
    WILDCARD,
    WILDCARD_HASH,
    FieldIndexType,
    FieldNames,
)
from hyperon_das_atomdb_cpp.database import AtomDB
from hyperon_das_atomdb_cpp.document_types import Atom, Link, Node

# pylint: disable=invalid-name

AtomT: TypeAlias = Atom
NodeT: TypeAlias = Node
LinkT: TypeAlias = Link

HandleT: TypeAlias = str

HandleListT: TypeAlias = list[HandleT]

HandleSetT: TypeAlias = set[HandleT]

IncomingLinksT: TypeAlias = HandleListT | list[AtomT]

# pylint: enable=invalid-name


__all__ = [
    "FieldNames",
    "FieldIndexType",
    "AtomDB",
    "WILDCARD",
    "WILDCARD_HASH",
    "TYPE_HASH",
    "TYPEDEF_MARK_HASH",
    "AtomT",
    "NodeT",
    "LinkT",
    "HandleT",
    "HandleListT",
    "HandleSetT",
    "IncomingLinksT",
]
