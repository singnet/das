from hyperon_das.commons.atoms import Atom
from hyperon_das.logger import log
from hyperon_das.hasher import Hasher, composite_hash
from hyperon_das.commons.properties import Properties
from hyperon_das.commons.atoms import HandleDecoder
from hyperon_das.query_answer import Assignment


class Link(Atom):
    def __init__(
        self,
        type: str | None = None,
        targets: list[str] | None = None,
        is_toplevel: bool = False,
        custom_attributes: Properties = Properties(),
        tokens: list[str] | None = None,
        other: 'Link' | None = None
    ) -> None:
        if tokens:
            self.untokenize(tokens)
        elif other:
            super().__init__(other=other)
            self.targets = other.targets
        else:
            if not type or not targets:
                raise ValueError("'type' and 'targets' are required")
            super().__init__(type, is_toplevel, custom_attributes)
            self.targets = targets
            self.validate()

    def __eq__(self, other: 'Link') -> bool:
        if not isinstance(other, Link):
            return False
        return (
            self.targets == other.targets
            and self.type == other.type
            and self.is_toplevel == other.is_toplevel
            and self.custom_attributes == other.custom_attributes
        )

    def __ne__(self, other: 'Link') -> bool:
        return not self.__eq__(other)

    def copy_from(self, other: 'Link') -> 'Link':
        super().copy_from(other)
        self.targets = other.targets
        return self

    def validate(self) -> None:
        if self.type == Atom.UNDEFINED_TYPE:
            log.error(f"Link type can't be '{Atom.UNDEFINED_TYPE}'")
            raise

        if len(self.targets) == 0:
            log.error("Link must have at least 1 target")
            raise

    def to_string(self) -> str:
        result = f"Link(type: '{self.type}', targets: ["

        if len(self.targets) != 0:
            for target in self.targets:
                result += target + ", "

            result = result[0: (len(result) - 2)]

        result += f"], is_toplevel: {'true' if self.is_toplevel else 'false'}, custom_attributes: {self.custom_attributes.to_string()}"

        return result

    def arity(self) -> int:
        return len(self.targets)

    def tokenize(self, output: list[str]) -> None:
        output[0:0] = self.targets
        output.insert(0, str(len(self.targets)))
        super().tokenize(output)

    def untokenize(self, tokens: list[str]) -> None:
        super().untokenize(tokens)
        num_targets = int(tokens[0])
        self.targets[0:0] = tokens[1: 1 + num_targets]
        del tokens[:1 + num_targets]

    def composite_type(self, decoder: 'HandleDecoder') -> list[str]:
        composite_type = []
        composite_type.append(self.named_type_hash())
        for handle in self.targets:
            atom = decoder.get_atom(handle)
            if atom is None:
                log.error(f"Unkown atom with handle: {handle}")
                return []
            else:
                composite_type.append(atom.composite_type_hash(decoder))
        return composite_type

    def composite_type_hash(self, decoder: 'HandleDecoder') -> str:
        composite_type = self.composite_type(decoder)
        return composite_hash(composite_type, len(composite_type))

    def handle(self) -> str:
        return Hasher.link_handle(self.type, self.targets)

    def metta_representation(self, decoder) -> str:
        if self.type != "Expression":
            log.error(f"Can't compute metta expression of link whose type ({self.type}) is not Expression")

        metta_string = "("
        size = len(self.targets)

        for i in range(size):
            atom = decoder.get_atom(self.targets[i])

            if (atom is None):
                log.error("Couldn't decode handle: {self.targets[i]}")
                return ""

            metta_string += atom.metta_representation(decoder)

            if (i != (size - 1)):
                metta_string += " "

        metta_string += ")"
        return metta_string

    def match(self, handle, assignment: 'Assignment', decode: 'HandleDecoder') -> bool:
        return self.handle() == handle
