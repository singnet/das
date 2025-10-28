from abc import ABC, abstractmethod
from hyperon_das.commons.atoms import Atom


class HandleDecoder(ABC):
    """Abstract interface to be implemented by classes capable of creating an Atom from a handle."""

    @abstractmethod
    def get_atom(self, handle: str) -> "Atom":
        """Return an Atom corresponding to the given handle."""
