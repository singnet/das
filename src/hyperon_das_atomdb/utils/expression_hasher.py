"""
This module provides utility functions for hashing expressions and generating unique identifiers.

It includes classes for computing hashes of various types of expressions, such as named types,
terminals, and composite expressions. The module uses the MD5 hashing algorithm to generate
hashes and provides methods for creating composite hashes from lists of elements.
"""

from hashlib import md5
from typing import Any


class ExpressionHasher:
    """Utility class for hashing various types of expressions."""

    compound_separator = " "

    @staticmethod
    def compute_hash(
        text: str,
    ) -> str:
        """
        Compute the MD5 hash of the given text.

        This method takes a string input and returns its MD5 hash as a hexadecimal string.
        It is used internally by the `ExpressionHasher` class to generate unique identifiers
        for various types of expressions.

        Args:
            text (str): The input text to be hashed.

        Returns:
            str: The MD5 hash of the input text as a hexadecimal string.
        """
        return md5(text.encode("utf-8")).digest().hex()

    @staticmethod
    def named_type_hash(name: str) -> str:
        """
        Compute the hash for a named type.

        This method generates a hash for the given named type using the MD5 hashing algorithm.
        It is used to create unique identifiers for named types in the `ExpressionHasher` class.

        Args:
            name (str): The name of the type to be hashed.

        Returns:
            str: The MD5 hash of the named type as a hexadecimal string.
        """
        return ExpressionHasher.compute_hash(name)

    @staticmethod
    def terminal_hash(named_type: str, terminal_name: str) -> str:
        """
        Compute the hash for a terminal expression.

        This method generates a hash for the given terminal expression using the MD5 hashing
        algorithm. It combines the named type and terminal name to create a unique identifier
        for the terminal expression.

        Args:
            named_type (str): The name of the type to be hashed.
            terminal_name (str): The name of the terminal to be hashed.

        Returns:
            str: The MD5 hash of the terminal expression as a hexadecimal string.
        """
        return ExpressionHasher.compute_hash(
            ExpressionHasher.compound_separator.join([named_type, terminal_name])
        )

    @staticmethod
    def expression_hash(named_type_hash: str, elements: list[str]) -> str:
        """
        Compute the hash for a composite expression.

        This method generates a hash for the given composite expression using the MD5 hashing
        algorithm. It combines the named type hash and a list of element hashes to create a
        unique identifier for the composite expression.

        Args:
            named_type_hash (str): The hash of the named type.
            elements (list[str]): A list of element hashes to be combined.

        Returns:
            str: The MD5 hash of the composite expression as a hexadecimal string.
        """
        return ExpressionHasher.composite_hash([named_type_hash, *elements])

    @staticmethod
    def composite_hash(hash_base: str | list[Any]) -> str:
        """
        Compute the composite hash for the given base.

        This method generates a composite hash using the MD5 hashing algorithm. It can take
        either a single string or a list of strings as the base. If a list is provided, the
        elements are joined with a separator before hashing.

        Args:
            hash_base (str | list[str]): The base for the composite hash, either a single string
                                         or a list of strings.

        Returns:
            str: The MD5 hash of the composite base as a hexadecimal string.
        """
        if isinstance(hash_base, str):
            return hash_base
        elif isinstance(hash_base, list):
            if len(hash_base) == 1:
                return hash_base[0]
            else:
                return ExpressionHasher.compute_hash(
                    ExpressionHasher.compound_separator.join(hash_base)
                )
        # TODO unreachable
        else:
            raise ValueError(
                f"Invalid base to compute composite hash: {type(hash_base)}: {hash_base}"
            )
