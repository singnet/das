import functools

from hyperon_das.commons.atoms import Atom
from hyperon_das.commons.atoms.handle_decoder import HandleDecoder
from hyperon_das.commons.helpers import error
from hyperon_das.commons.properties import Properties
from hyperon_das.hasher import Hasher
from hyperon_das.query_answer import Assignment


class Node(Atom):
    def __init__(
        self,
        type: str | None = None,
        name: str | None = None,
        is_toplevel: bool = False,
        custom_attributes: Properties | None = None,
        tokens: list[str] | None = None,
        other=None,
    ) -> None:
        if tokens:
            self.custom_attributes = custom_attributes if custom_attributes is not None else Properties()
            self.untokenize(tokens)
        elif other:
            self._name = other.name
            super().__init__(other=other)
        else:
            if not type or not name:
                raise ValueError("'type' and 'name' are required")
            self._name = name
            super().__init__(type, is_toplevel, custom_attributes)

    @property
    def name(self) -> str:
        return self._name

    @name.setter
    def name(self, value: str) -> None:
        self._name = value

        # If the handle has already been calculated, delete it.
        if "handle" in self.__dict__:
            del self.__dict__["handle"]

    def __eq__(self, other: 'Node') -> bool:
        if not isinstance(other, Node):
            return False
        return (
            self.name == other.name
            and self.type == other.type
            and self.is_toplevel == other.is_toplevel
            and self.custom_attributes == other.custom_attributes
        )

    def __ne__(self, other: 'Node') -> bool:
        return not self.__eq__(other)

    def copy_from(self, other: 'Node') -> 'Node':
        super().copy_from(other)
        self.name = other.name
        return self

    def validate(self) -> None:
        if self.type == Atom.UNDEFINED_TYPE:
            error(f"Node type can't be '{Atom.UNDEFINED_TYPE}'")

        if len(self.name) == 0:
            error("Node name must not be empty")

    def to_string(self) -> str:
        return (
            f"Node(type: '{self.type}', name: '{self.name}', custom_attributes: '{self.custom_attributes.to_string()}'"
        )

    def tokenize(self, output: list[str]) -> None:
        super().tokenize(output)
        output.append(self.name)

    def untokenize(self, tokens: list[str]) -> None:
        super().untokenize(tokens)
        self.name = tokens[0]
        del tokens[:1]

    @functools.cached_property
    def handle(self) -> str:
        return Hasher.node_handle(self.type, self.name)

    def metta_representation(self, decoder: 'HandleDecoder') -> str:
        if self.type != "Symbol":
            error(f"Can't compute metta expression of node whose type ({self.type}) is not Symbol")
        return self.name

    def match(self, handle, assignment: 'Assignment', decode: 'HandleDecoder') -> bool:
        return self.handle == handle
