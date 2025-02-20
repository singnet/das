"""This module initializes the AtomDB package and imports key components."""

# pylint: disable=wrong-import-position

import sys

if sys.version_info < (3, 10):
    raise RuntimeError("hyperon-das-atomdb requires Python 3.10 or higher")

from .database import WILDCARD, AtomDB
from .exceptions import AtomDoesNotExist

__all__ = [
    "AtomDB",
    "WILDCARD",
    "AtomDoesNotExist",
]

__version__ = "0.8.10"
