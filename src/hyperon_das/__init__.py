import sys

if sys.version_info < (3, 10):
    raise RuntimeError("hyperon_das requires Python 3.10 or higher")

from hyperon_das.das import DistributedAtomSpace

__all__ = ["DistributedAtomSpace"]

__version__ = "0.9.14"
