import dataclasses
from abc import ABC, abstractmethod
from typing import Any, Type, TypeAlias

TOKENS_DELIMITER = " "


@dataclasses.dataclass
class Element(ABC):
    """An abstract class representing an element in the tokenizer."""

    @abstractmethod
    def to_tokens(self) -> list[str]:
        """
        Convert the element to a list of tokens.

        Returns:
            list[str]: A list of string tokens representing the element.
        """
        raise NotImplementedError

    @staticmethod
    @abstractmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, "Element"]:
        """
        Create an Element instance from a list of tokens.

        Args:
            tokens (list[str]): The list of tokens to parse.
            cursor (int, optional): The starting position in the token list. Defaults to 0.

        Returns:
            tuple[int, Element]: A tuple containing the updated cursor position and the created
                                 Element instance.

        Raises:
            ValueError: If the tokens do not represent a valid Element.
        """
        raise NotImplementedError


class ElementBuilder:
    """A class for creating instances of Elements from tokens."""

    ElementsMappingKey: TypeAlias = str
    ElementsMappingValue: TypeAlias = Type[Element]
    elements_mapping: dict[ElementsMappingKey, ElementsMappingValue] = {}
    """A mapping of tokens tags to Element types."""

    @classmethod
    def register_tag(cls, tag: str) -> TypeAlias:
        """
        A decorator for registering an element type with a specific tag.

        Args:
            tag (str): The tag associated with the element type.

        Returns:
            TypeAlias: The decorated element type.
        """

        def decorator(element_type: Type[Element]) -> Type[Element]:
            cls.elements_mapping[tag] = element_type
            return element_type

        return decorator

    @staticmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, Element]:
        """
        Create an instance of a class from a list of tokens.

        Args:
            tokens (list[str]): The list of tokens to parse, where the first token (the one at the
                `cursor` position) is supposed to be a registered tag.
            cursor (int, optional): The starting position in the token list. Defaults to 0.

        Returns:
            tuple[int, Any]: A tuple containing the updated cursor position and the created instance.

        Raises:
            ValueError: If the tokens do not represent a valid Element.
        """
        if element := ElementBuilder.elements_mapping.get(tokens[cursor]):
            return element.from_tokens(tokens, cursor)
        raise ValueError(f"Unsupported sequence of tokens for MAIN BUILDER: {tokens[cursor:]}")


@dataclasses.dataclass
@ElementBuilder.register_tag("NODE")
class Node(Element):
    """
    A class representing a node in the tokenizer.

    Attributes:
        type (str): The type of the node.
        name (str): The name of the node.
    """

    type: str
    name: str

    def to_tokens(self) -> list[str]:
        return ["NODE", self.type, self.name]

    @staticmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, "Node"]:
        if tokens[cursor] == "NODE":
            cursor += 1  # Skip the "NODE" token
            node = Node(type=tokens[cursor], name=tokens[cursor + 1])
            cursor += 2  # Skip the type and name tokens
            return cursor, node
        raise ValueError(f"Unsupported sequence of tokens for NODE: {tokens[cursor:]}")


@dataclasses.dataclass
@ElementBuilder.register_tag("HANDLE")
class Handle(Element):
    """
    A class representing a node in the tokenizer.

    Attributes:
        type (str): The type of the node.
        name (str): The name of the node.
    """

    value: str

    def to_tokens(self) -> list[str]:
        return ["HANDLE", self.value]

    @staticmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, "Node"]:
        if tokens[cursor] == "HANDLE":
            cursor += 1  # Skip the "NODE" token
            node = Handle(value=tokens[cursor])
            cursor += 1  # Skip the value
            return cursor, node
        raise ValueError(f"Unsupported sequence of tokens for NODE: {tokens[cursor:]}")


@dataclasses.dataclass
@ElementBuilder.register_tag("VARIABLE")
class Variable(Element):
    """
    A class representing a variable in the tokenizer.

    Attributes:
        name (str): The name of the variable.
    """

    name: str

    def to_tokens(self) -> list[str]:
        return ["VARIABLE", self.name]

    @staticmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, "Variable"]:
        if tokens[cursor] == "VARIABLE":
            cursor += 1  # Skip the "VARIABLE" token
            variable = Variable(name=tokens[cursor])
            cursor += 1  # Skip the name token
            return cursor, variable
        raise ValueError(f"Unsupported sequence of tokens for VARIABLE: {tokens[cursor:]}")


@dataclasses.dataclass
@ElementBuilder.register_tag("LINK")
class Link(Element):
    """
    A class representing a link in the tokenizer.

    Attributes:
        type (str): The type of the link.
        targets (list[Link | Node | Variable]): A list of targets associated with the link.
        is_template (bool): Indicates if the link is a template. Defaults to False.
    """

    type: str
    targets: list[Any] = dataclasses.field(default_factory=list)
    is_template: bool = False
    custom_field: dict[Any] = dataclasses.field(default_factory=dict)

    def to_tokens(self) -> list[str]:
        self.is_template = any(isinstance(target, Variable) for target in self.targets)
        return [
            "LINK_TEMPLATE" if self.is_template else "LINK",
            self.type,
            str(len(self.targets)),
            *[token for target in self.targets for token in target.to_tokens()],
        ]

    @staticmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, "Link"]:
        if tokens[cursor] in ("LINK", "LINK_TEMPLATE"):
            link_tag = tokens[cursor]
            cursor += 1  # Skip the "LINK" or "LINK_TEMPLATE" token
            link = Link(type=tokens[cursor])
            cursor += 1  # Skip the type token
            target_count = int(tokens[cursor])
            if target_count < 2:
                raise ValueError("Link requires at least two targets")
            cursor += 1  # Skip the target count token
            for _ in range(target_count):
                cursor, target = ElementBuilder.from_tokens(tokens, cursor)
                if isinstance(target, Variable):
                    link.is_template = True
                link.targets.append(target)
            if tokens[cursor] == "CUSTOM_FIELD":
                cursor, custom_field = ElementBuilder.from_tokens(tokens, cursor)
                link.custom_field = dict(custom_field)
            if link_tag == "LINK_TEMPLATE" and not link.is_template:
                raise ValueError("Template link without variables")
            elif link_tag == "LINK" and link.is_template:
                raise ValueError("Non-template link with variables")
            return cursor, link
        raise ValueError(f"Unsupported sequence of tokens for LINK: {tokens[cursor:]}")


@dataclasses.dataclass
@ElementBuilder.register_tag("LINK_TEMPLATE")
class LinkTemplate(Link):
    """
    A class representing a template link in the tokenizer.

    Inherits from the Link class and is used to denote links that are templates.
    """

    def __init__(self, type: str, targets: list[Any]) -> None:
        if not targets:
            targets = []
        super().__init__(type, targets, is_template=True)


@dataclasses.dataclass
@ElementBuilder.register_tag("OR")
class OrOperator(Element):
    """
    A class representing an OR operator in the tokenizer.

    Attributes:
        operands (list[Element]): A list of operands associated with the OR operator.
    """

    operands: list[Element] = dataclasses.field(default_factory=list)

    def to_tokens(self) -> list[str]:
        return [
            "OR",
            str(len(self.operands)),
            *[token for operand in self.operands for token in operand.to_tokens()],
        ]

    @staticmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, "OrOperator"]:
        if tokens[cursor] == "OR":
            cursor += 1  # Skip the "OR" token
            operator = OrOperator()
            operand_count = int(tokens[cursor])
            if operand_count < 2:
                raise ValueError("OR operator requires at least two operands")
            cursor += 1  # Skip the operand count token
            for _ in range(operand_count):
                cursor, operand = ElementBuilder.from_tokens(tokens, cursor)
                operator.operands.append(operand)
            return cursor, operator
        raise ValueError(f"Unsupported sequence of tokens for OR: {tokens[cursor:]}")


@dataclasses.dataclass
@ElementBuilder.register_tag("AND")
class AndOperator(Element):
    """
    A class representing an AND operator in the tokenizer.

    Attributes:
        operands (list[Element]): A list of operands associated with the AND operator.
    """

    operands: list[Element] = dataclasses.field(default_factory=list)

    def to_tokens(self) -> list[str]:
        return [
            "AND",
            str(len(self.operands)),
            *[token for operand in self.operands for token in operand.to_tokens()],
        ]

    @staticmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, "AndOperator"]:
        if tokens[cursor] == "AND":
            cursor += 1  # Skip the "AND" token
            operator = AndOperator()
            operand_count = int(tokens[cursor])
            if operand_count < 2:
                raise ValueError("AND operator requires at least two operands")
            cursor += 1  # Skip the operand count token
            for _ in range(operand_count):
                cursor, operand = ElementBuilder.from_tokens(tokens, cursor)
                operator.operands.append(operand)
            return cursor, operator
        raise ValueError(f"Unsupported sequence of tokens for AND: {tokens[cursor:]}")


@dataclasses.dataclass
@ElementBuilder.register_tag("NOT")
class NotOperator(Element):
    """
    A class representing a NOT operator in the tokenizer.

    Attributes:
        operand (Element): The operand associated with the NOT operator.
    """

    operand: Element

    def to_tokens(self) -> list[str]:
        return ["NOT", *self.operand.to_tokens()]

    @staticmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, "NotOperator"]:
        if tokens[cursor] == "NOT":
            cursor += 1  # Skip the "NOT" token
            cursor, operand = ElementBuilder.from_tokens(tokens, cursor)
            return cursor, NotOperator(operand)
        raise ValueError(f"Unsupported sequence of tokens for NOT: {tokens[cursor:]}")


@dataclasses.dataclass
@ElementBuilder.register_tag("CUSTOM_FIELD")
class CustomField(Element):
    type: str
    fields: dict[Any] = dataclasses.field(default_factory=dict)

    def to_tokens(self) -> list[str]:
        # self.is_template = any(isinstance(target, Variable) for target in self.targets)
        return []

    @staticmethod
    def from_tokens(tokens: list[str], cursor: int = 0) -> tuple[int, "CustomField"]:
        if tokens[cursor] == "CUSTOM_FIELD":
            # custom_field_tag = tokens[cursor]
            cursor += 1  # Skip the "CUSTOM_FIELD" token
            custom_field = CustomField(type=tokens[cursor])
            cursor += 1  # Skip the type token
            fields_count = int(tokens[cursor])
            # if target_count < 2:
            #     raise ValueError("Link requires at least two targets")
            cursor += 1  # Skip the field count token
            for _ in range(fields_count):
                if tokens[cursor] == "CUSTOM_FIELD":
                    cursor, field = ElementBuilder.from_tokens(tokens, cursor)
                    custom_field.fields[field.type] = field
                else:
                    custom_field.fields[tokens[cursor]] = tokens[cursor + 1]
                    cursor += 2
            return cursor, custom_field
        raise ValueError(f"Unsupported sequence of tokens for LINK: {tokens[cursor:]}")

    def __iter__(self):
        cf_dict = {}
        tmp = {}
        for key, value in self.fields.items():
            if isinstance(value, Element):
                tmp[key] = dict(value)[key]
            else:
                tmp[key] = value
        cf_dict[self.type] = tmp
        return iter(cf_dict.items())
