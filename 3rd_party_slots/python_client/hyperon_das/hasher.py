import hashlib
from hyperon_das.logger import log


JOINING_CHAR = ' '
MAX_LITERAL_OR_SYMBOL_SIZE = 10000
MAX_HASHABLE_STRING_SIZE = 100000
HANDLE_HASH_SIZE = 33


def compute_hash(input: str) -> str:
    return hashlib.md5(input.encode("utf-8")).hexdigest()


def named_type_hash(name: str) -> str:
    return compute_hash(name)


def terminal_hash(type: str, name: str) -> str:
    if (len(type) + len(name)) >= MAX_HASHABLE_STRING_SIZE:
        log.error("Invalid (too large) terminal name")
        raise ValueError

    hashable_string = f"{type}{JOINING_CHAR}{name}"

    return compute_hash(hashable_string)


def composite_hash(elements: list[str], nelements: int) -> str:
    total_size = 0
    element_size = []

    for i in range(nelements):
        size = len(elements[i])
        if size > MAX_LITERAL_OR_SYMBOL_SIZE:
            log.error("Invalid (too large) composite elements")
            raise ValueError

        element_size.append(size)
        total_size += size

    if total_size >= MAX_HASHABLE_STRING_SIZE:
        log.error("Invalid (too large) composite elements")
        raise ValueError

    hashable_string = JOINING_CHAR.join(elements[:nelements])

    return compute_hash(hashable_string)


def expression_hash(type_hash: str, elements: list[str]) -> str:
    nelements = len(elements)
    composite = [type_hash] + elements[:nelements]
    return composite_hash(composite, nelements + 1)


class Hasher:
    @staticmethod
    def plain_string_hash(s: str) -> str:
        return named_type_hash(s)

    @staticmethod
    def context_handle(name: str) -> str:
        return Hasher.plain_string_hash(name)

    @staticmethod
    def type_handle(type_name: str) -> str:
        return Hasher.plain_string_hash(type_name)

    @staticmethod
    def node_handle(type_name: str, name: str) -> str:
        return terminal_hash(type_name, name)

    @staticmethod
    def link_handle(type_name: str, targets: list[str]) -> str:
        type_h = named_type_hash(type_name)
        return expression_hash(type_h, targets)

    @staticmethod
    def composite_handle(elements: list[str]) -> str:
        return composite_hash(elements, len(elements))
