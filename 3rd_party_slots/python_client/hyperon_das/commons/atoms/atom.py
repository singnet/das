import abc
import copy

from hyperon_das.commons.properties import Properties
from hyperon_das.hasher import Hasher
from hyperon_das.commons.atoms.handle_decoder import HandleDecoder
from hyperon_das.query_answer import Assignment
from hyperon_das.commons.helpers import error


class Atom:
    UNDEFINED_TYPE = "__UNDEFINED_TYPE__"
    WILDCARD_STRING = "*"

    def __init__(
        self,
        type: str | None = None,
        is_toplevel: bool = False,
        custom_attributes: Properties | None = None,
        other=None
    ) -> None:
        if other:
            self.type = other.type
            self.is_toplevel = other.is_toplevel
            self.custom_attributes = copy.deepcopy(other.custom_attributes)
        else:
            self.type = type
            self.is_toplevel = is_toplevel
            self.custom_attributes = custom_attributes if custom_attributes is not None else Properties()
            self.validate()

    def __eq__(self, other: 'Atom') -> bool:
        if not isinstance(other, Atom):
            return False
        return (
            self.type == other.type
            and self.is_toplevel == other.is_toplevel
            and self.custom_attributes == other.custom_attributes
        )

    def __ne__(self, other: 'Atom') -> bool:
        return not self.__eq__(other)

    def copy_from(self, other: 'Atom') -> 'Atom':
        self.type = other.type
        self.is_toplevel = other.is_toplevel
        self.custom_attributes = copy.deepcopy(other.custom_attributes)
        return self

    @staticmethod
    def is_node(atom: 'Atom') -> bool:
        return atom.arity() == 0

    @staticmethod
    def is_link(atom: 'Atom') -> bool:
        return atom.arity() > 0

    def validate(self) -> None:
        if len(self.type) == 0:
            error("Atom type must not be empty")

    def to_string(self) -> str:
        return f"Atom(type: '{self.type}', is_toplevel: {str(self.is_toplevel)}, custom_attributes: {self.custom_attributes.to_string()})"

    def named_type_hash(self) -> str:
        return Hasher.type_handle(self.type)

    def composite_type(self, decoder: 'HandleDecoder') -> list[str]:
        return [self.named_type_hash()]

    def composite_type_hash(self, decoder: 'HandleDecoder') -> str:
        return self.named_type_hash()

    def schema_handle(self) -> str:
        return self.handle()

    def arity(self) -> int:
        return 0

    def tokenize(self, output: list[str]) -> None:
        parameters_tokens = self.custom_attributes.tokenize()
        parameters_tokens.insert(0, str(len(parameters_tokens)))
        output[0:0] = parameters_tokens
        output.insert(0, "true" if self.is_toplevel else "false")
        output.insert(0, self.type)

    def untokenize(self, tokens: list[str]) -> None:
        self.type = tokens[0]
        self.is_toplevel = tokens[1] == "true"
        del tokens[:2]

        num_property_tokens = int(tokens[0])
        if num_property_tokens > 0:
            properties_tokens = tokens[1: 1 + num_property_tokens]
            self.custom_attributes.untokenize(properties_tokens)
            del tokens[: 1 + num_property_tokens]
        else:
            del tokens[0]

    @abc.abstractmethod
    def handle(self) -> str:
        """Constructs and returns this Atom's handle."""

    @abc.abstractmethod
    def metta_representation(self, decoder: 'HandleDecoder') -> str:
        """ Constructs and returns a MeTTa expression which represents this Atom."""

    @abc.abstractmethod
    def match(self, handle, assignment: 'Assignment', decode: 'HandleDecoder') -> bool:
        """
        Match atoms against the passed handle and return true iff there's a match.

        If a variable assignment is required in order to make the match (e.g. for variables and
        link_schemas), this assigment is returned by side-effect.
        """
