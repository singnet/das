from abc import ABC, abstractmethod


class HandleDecoder(ABC):
    """Abstract interface to be implemented by classes capable of creating an Atom from a handle."""

    @abstractmethod
    def get_atom(self, handle: str):
        """Return an Atom corresponding to the given handle."""
