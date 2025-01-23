from abc import ABC, abstractmethod
from typing import Any, Dict, List, Optional

from hyperon_das_atomdb import AtomDB


class PatternMatchingAnswer:
    def __init__(self):
        pass


class LogicalExpression(ABC):
    @abstractmethod
    def matched(
        self,
        db: AtomDB,
        answer: PatternMatchingAnswer,
        extra_parameters: Optional[Dict[str, Any]] = None,
    ) -> bool:
        pass


class Atom(LogicalExpression, ABC):
    def __init__(self, atom_type: str):
        pass

    @abstractmethod
    def get_handle(self, db: AtomDB) -> str:
        pass


class Node(Atom):
    def __init__(self, node_type: str, node_name: str):
        pass

    def get_handle(self, db: AtomDB) -> str:
        return ""

    def matched(
        self,
        db: AtomDB,
        answer: PatternMatchingAnswer,
        extra_parameters: Optional[Dict[str, Any]] = None,
    ) -> bool:
        return True


class Link(Atom):
    def __init__(self, link_type: str, targets: List[Atom], ordered: bool):
        pass

    def get_handle(self, db: AtomDB) -> str:
        return ""

    def matched(
        self,
        db: AtomDB,
        answer: PatternMatchingAnswer,
        extra_parameters: Optional[Dict[str, Any]] = None,
    ) -> bool:
        return True


class Variable(Atom):
    def __init__(self, variable_name: str):
        pass

    def get_handle(self, db: AtomDB) -> str:
        return ""

    def matched(
        self,
        db: AtomDB,
        answer: PatternMatchingAnswer,
        extra_parameters: Optional[Dict[str, Any]] = None,
    ) -> bool:
        return True


class Not(LogicalExpression):
    def __init__(self, term: LogicalExpression):
        pass

    def matched(
        self,
        db: AtomDB,
        answer: PatternMatchingAnswer,
        extra_parameters: Optional[Dict[str, Any]] = None,
    ) -> bool:
        return True


class Or(LogicalExpression):
    def __init__(self, terms: List[LogicalExpression]):
        pass

    def matched(
        self,
        db: AtomDB,
        answer: PatternMatchingAnswer,
        extra_parameters: Optional[Dict[str, Any]] = None,
    ) -> bool:
        return True


class And(LogicalExpression):
    def __init__(self, terms: List[LogicalExpression]):
        pass

    def matched(
        self,
        db: AtomDB,
        answer: PatternMatchingAnswer,
        extra_parameters: Optional[Dict[str, Any]] = None,
    ) -> bool:
        return True
