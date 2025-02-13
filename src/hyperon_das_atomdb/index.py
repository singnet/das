"""This module contains the abstract class for index creation and management."""

from abc import ABC, abstractmethod
from typing import Any

from hyperon_das_atomdb.utils.expression_hasher import ExpressionHasher


class Index(ABC):
    """Abstract class for index creation and management."""

    @staticmethod
    def generate_index_id(field: str, conditionals: dict[str, Any]) -> str:
        """Generates an index ID based on the field name.

        Args:
            field (str): The field name.
            conditionals (dict[str, Any]): The conditionals.

        Returns:
            str: The index ID.
        """
        return ExpressionHasher.compute_hash(f"{field}{conditionals}")

    @abstractmethod
    def create(
        self,
        atom_type: str,
        fields: list[str],
        **kwargs,
    ) -> tuple[str, Any]:
        """Creates an index on the given field.

        Args:
            atom_type (str): Atom's type
            fields (list[str]): The fields to create the index on.


        Returns:
            tuple[str, Any]: Returns the index id and the index properties dict
        """

    @abstractmethod
    def index_exists(self, index_id: str) -> bool:
        """Checks if an index exists

        Args:
            index_id (str): The index ID.

        Returns:
            bool: True if the index exists, False otherwise.
        """
