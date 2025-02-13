"""Custom exceptions for Atom DB"""

from hyperon_das_atomdb_cpp.exceptions import (
    AddLinkException,
    AddNodeException,
    AtomDbBaseException,
    AtomDoesNotExist,
    InvalidAtomDB,
    InvalidOperationException,
    RetryException,
)


class ConnectionMongoDBException(AtomDbBaseException):
    """Exception raised for errors in the connection to MongoDB."""


__all__ = [
    "ConnectionMongoDBException",
    "AtomDbBaseException",
    "AtomDoesNotExist",
    "AddNodeException",
    "AddLinkException",
    "InvalidOperationException",
    "RetryException",
    "InvalidAtomDB",
]
