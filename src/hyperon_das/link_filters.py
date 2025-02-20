from dataclasses import dataclass
from enum import Enum, auto

from hyperon_das_atomdb import WILDCARD


class LinkFilterType(int, Enum):
    NAMED_TYPE = auto()
    FLAT_TYPE_TEMPLATE = auto()
    TARGETS = auto()


@dataclass
class LinkFilter:
    filter_type: LinkFilterType
    toplevel_only: bool
    link_type: str = None
    target_types: list[str] = None
    targets: list[str] = None


@dataclass
class NamedType(LinkFilter):
    """
    All links of a given type. Optionally, only toplevel links are selected.
    """

    def __init__(self, link_type: str, toplevel_only: bool = False):
        self.filter_type = LinkFilterType.NAMED_TYPE
        self.link_type = link_type
        self.toplevel_only = toplevel_only


@dataclass
class FlatTypeTemplate(LinkFilter):
    """
    All links of a given type whose targets are of given types. Any number of wildcards '*' can
    be used as link or target types.
    """

    def __init__(
        self,
        target_types: list[str],
        link_type: str = WILDCARD,
        toplevel_only: bool = False,
    ):
        self.filter_type = LinkFilterType.FLAT_TYPE_TEMPLATE
        self.link_type = link_type
        self.target_types = target_types
        self.toplevel_only = toplevel_only


@dataclass
class Targets(LinkFilter):
    """
    All links of a given type whose targets match a given list of handles. Any number of
    wildcards '*' can be used as link type or target handle.
    """

    def __init__(self, targets: list[str], link_type: str = WILDCARD, toplevel_only: bool = False):
        self.filter_type = LinkFilterType.TARGETS
        self.link_type = link_type
        self.targets = targets
        self.toplevel_only = toplevel_only
