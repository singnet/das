import re
from collections import OrderedDict
from typing import Any, List, Optional, Tuple
from unittest.mock import patch

from hyperon_das_atomdb import WILDCARD, AtomDB
from hyperon_das_atomdb.database import (
    AtomT,
    FieldIndexType,
    HandleListT,
    IncomingLinksT,
    LinkT,
    NodeT,
)

from hyperon_das import DistributedAtomSpace
from hyperon_das.cache.cache_controller import CacheController
from hyperon_das.das import LocalQueryEngine, RemoteQueryEngine


def _build_node_handle(node_type: str, node_name: str) -> str:
    return f"<{node_type}: {node_name}>"


def _split_node_handle(node_handle: str) -> Tuple[str, str]:
    v = re.split("[<: >]", node_handle)
    return v[1], v[3]


def _build_link_handle(link_type: str, target_handles: List[str]) -> str:
    if link_type == "Similarity" or link_type == "Set":
        target_handles.sort()
    return f"<{link_type}: {target_handles}>"


class DistributedAtomSpaceMock(DistributedAtomSpace):
    def __init__(self, query_engine: Optional[str] = "local", **kwargs) -> None:
        self.backend = DatabaseAnimals()
        self.cache_controller = CacheController({})
        if query_engine == "remote":
            with patch("hyperon_das.client.connect_to_server", return_value=(200, "OK")):
                self.query_engine = RemoteQueryEngine(
                    self.backend, self.cache_controller, {}, **kwargs
                )
        else:
            self.query_engine = LocalQueryEngine(self.backend, self.cache_controller, {}, **kwargs)


class DatabaseMock(AtomDB):
    def __init__(self):
        super().__init__()
        self.node_names = [
            "human",
            "monkey",
            "chimp",
            "snake",
            "earthworm",
            "rhino",
            "triceratops",
            "vine",
            "ent",
            "mammal",
            "animal",
            "reptile",
            "dinosaur",
            "plant",
        ]
        self.node_type = "Concept"
        self.all_nodes: dict[str, dict[str, str]] = {
            _build_node_handle(self.node_type, name): {
                "type": self.node_type,
                "name": name,
            }
            for name in self.node_names
        }

        self.human = _build_node_handle(self.node_type, "human")
        self.monkey = _build_node_handle(self.node_type, "monkey")
        self.chimp = _build_node_handle(self.node_type, "chimp")
        self.snake = _build_node_handle(self.node_type, "snake")
        self.earthworm = _build_node_handle(self.node_type, "earthworm")
        self.rhino = _build_node_handle(self.node_type, "rhino")
        self.triceratops = _build_node_handle(self.node_type, "triceratops")
        self.vine = _build_node_handle(self.node_type, "vine")
        self.ent = _build_node_handle(self.node_type, "ent")
        self.mammal = _build_node_handle(self.node_type, "mammal")
        self.animal = _build_node_handle(self.node_type, "animal")
        self.reptile = _build_node_handle(self.node_type, "reptile")
        self.dinosaur = _build_node_handle(self.node_type, "dinosaur")
        self.plant = _build_node_handle(self.node_type, "plant")

        self.all_links: list[list[str]] = [
            ["Similarity", self.human, self.monkey],
            ["Similarity", self.human, self.chimp],
            ["Similarity", self.chimp, self.monkey],
            ["Similarity", self.snake, self.earthworm],
            ["Similarity", self.rhino, self.triceratops],
            ["Similarity", self.snake, self.vine],
            ["Similarity", self.human, self.ent],
            ["Inheritance", self.human, self.mammal],
            ["Inheritance", self.monkey, self.mammal],
            ["Inheritance", self.chimp, self.mammal],
            ["Inheritance", self.mammal, self.animal],
            ["Inheritance", self.reptile, self.animal],
            ["Inheritance", self.snake, self.reptile],
            ["Inheritance", self.dinosaur, self.reptile],
            ["Inheritance", self.triceratops, self.dinosaur],
            ["Inheritance", self.earthworm, self.animal],
            ["Inheritance", self.rhino, self.mammal],
            ["Inheritance", self.vine, self.plant],
            ["Inheritance", self.ent, self.plant],
            [
                "List",
                _build_link_handle("Inheritance", [self.dinosaur, self.reptile]),
                _build_link_handle("Inheritance", [self.triceratops, self.dinosaur]),
            ],
            [
                "Set",
                _build_link_handle("Inheritance", [self.dinosaur, self.reptile]),
                _build_link_handle("Inheritance", [self.triceratops, self.dinosaur]),
            ],
            ["List", self.human, self.ent, self.monkey, self.chimp],
            ["List", self.human, self.mammal, self.triceratops, self.vine],
            ["List", self.human, self.monkey, self.chimp],
            ["List", self.triceratops, self.ent, self.monkey, self.snake],
            ["Set", self.triceratops, self.vine, self.monkey, self.snake],
            ["Set", self.triceratops, self.ent, self.monkey, self.snake],
            ["Set", self.human, self.ent, self.monkey, self.chimp],
            ["Set", self.mammal, self.monkey, self.human, self.chimp],
            ["Set", self.human, self.monkey, self.chimp],
        ]

        self.template_index: dict[str, list[tuple[str, tuple[str, ...]]]] = {}
        self.incoming_set: dict[str, list[str]] = {}

        for link in self.all_links:
            key = [link[0]]
            for target in link[1:]:
                node_type, node_name = _split_node_handle(target)
                key.append(node_type)
            key = str(key)
            self.template_index.setdefault(key, []).append(
                (_build_link_handle(link[0], link[1:]), tuple(link[1:]))
            )
            self._add_incoming_set(str(link), link[1:])

        nested_link = [
            "Evaluation",
            self.human,
            [
                "Evaluation",
                self.human,
                _build_link_handle("Set", [self.monkey, self.mammal]),
            ],
        ]
        self.all_links.append(nested_link)  # type: ignore

    def __repr__(self) -> str:
        return "<Atom database Mock>"

    def _add_incoming_set(self, key: str, targets: list[str]):
        for target in targets:
            self.incoming_set.setdefault(target, list()).append(key)

    def node_exists(self, node_type: str, node_name: str) -> bool:
        return _build_node_handle(node_type, node_name) in self.all_nodes

    def link_exists(self, link_type: str, target_handles: list[str]) -> bool:
        return _build_link_handle(link_type, target_handles) in [
            _build_link_handle(link[0], link[1:]) for link in self.all_links
        ]

    def get_node_handle(self, node_type: str, node_name: str) -> str:
        node_handle = _build_node_handle(node_type, node_name)
        for node in self.all_nodes:
            if node == node_handle:
                return node

    @staticmethod
    def node_handle(node_type: str, node_name: str) -> str:
        return _build_node_handle(node_type, node_name)

    @staticmethod
    def link_handle(link_type: str, target_handles: list[str]) -> str:
        return _build_link_handle(link_type, target_handles)

    def _get_atom(self, handle: str) -> AtomT | None:
        # TODO: must revisit this and build a proper AtomT to return
        for node in self.all_nodes:
            if node == handle:
                return node
        for link in self.all_links:
            return link

    def is_ordered(self, link_handle: str) -> bool:
        for link in self.all_links:
            if _build_link_handle(link[0], link[1:]) == link_handle:
                return link[0] != "Similarity" and link[0] != "Set"
        return True

    def get_link_handle(self, link_type: str, target_handles: list[str]) -> str:
        for link in self.all_links:
            if link[0] == link_type and len(target_handles) == (len(link) - 1):
                if link_type == "Similarity":
                    if all(target in target_handles for target in link[1:]):
                        return _build_link_handle(link_type, link[1:])
                elif link_type == "Inheritance":
                    for i in range(0, len(target_handles)):
                        if target_handles[i] != link[i + 1]:
                            break
                    else:
                        return _build_link_handle(link_type, target_handles)
                else:
                    raise ValueError(f"Invalid link type: {link_type}")

    def get_link_targets(self, link_handle: str) -> list[str]:
        for link in self.all_links:
            if _build_link_handle(link[0], link[1:]) == link_handle:
                return link[1:]

    def get_matched_links(self, link_type: str, target_handles: list[str], **kwargs) -> HandleListT:
        answer = []
        for link in self.all_links:
            if len(target_handles) == (len(link) - 1) and link[0] == link_type:
                if link[0] == "Similarity" or link[0] == "Set":
                    if all(target == WILDCARD or target in link[1:] for target in target_handles):
                        answer.append(_build_link_handle(link[0], link[1:]))
                elif link[0] == "Inheritance" or link[0] == "List":
                    for i in range(0, len(target_handles)):
                        if target_handles[i] != WILDCARD and target_handles[i] != link[i + 1]:
                            break
                    else:
                        answer.append(_build_link_handle(link[0], []))
                elif link[0] == "Evaluation":
                    answer.append("test")
                else:
                    raise ValueError(f"Invalid link type: {link[0]}")
        return answer

    def get_all_nodes_handles(self, node_type: str) -> list[str]:
        return self.all_nodes.keys() if node_type == self.node_type else []

    def get_all_nodes_names(self, node_type: str) -> list[str]:
        return (
            [node["name"] for node in self.all_nodes.values()]
            if node_type == self.node_type
            else []
        )

    def get_all_links(self, link_type: str, **kwargs) -> tuple[int | None, list[str]]:
        raise NotImplementedError()

    def get_matched_type_template(self, template: list[Any], **kwargs) -> HandleListT:
        assert len(template) == 3
        return self.template_index.get(str(template))

    def get_node_name(self, node_handle: str) -> str:
        _, name = _split_node_handle(node_handle)
        return name

    def _get_matched_node_name(self, node_type: str, substring: str) -> list[str]:
        answer = []
        if node_type == self.node_type:
            for node in self.all_nodes:
                _, name = _split_node_handle(node)
                if substring in name:
                    answer.append(node)
        return answer

    def get_matched_type(self, link_type: str, **kwargs) -> HandleListT:
        raise NotImplementedError()

    def get_atom_as_dict(self, handle: str, arity: int | None = 0) -> dict[str, Any]:
        if handle in self.all_nodes:
            return {
                "handle": handle,
                "type": handle.split()[0][1:-1],
                "name": handle.split()[1][:-1],
            }
        match = re.search(r"<([^:]+): (.+)>", handle)
        _type = match.group(1)
        targets = eval(match.group(2))
        template = [_type]
        for target in targets:
            template.append(_split_node_handle(target)[0])
        if match:
            return {
                "handle": handle,
                "type": _type,
                "template": template,
                "targets": targets,
            }

    def get_link_type(self, link_handle: str) -> str | None:
        document = self.get_atom_as_dict(link_handle)
        return document["type"]

    def get_node_type(self, node_handle: str) -> str | None:
        document = self.get_atom_as_dict(node_handle)
        return document["type"]

    def count_atoms(self, parameters: dict[str, Any] | None = None) -> dict[str, int]:
        return {
            "link_count": len(self.all_links),
            "node_count": len(self.all_nodes),
            "atom_count": len(self.all_links) + len(self.all_nodes),
        }
        # return (len(self.all_nodes), len(self.all_links))

    def clear_database(self):
        raise NotImplementedError()

    def add_link(self, link_params: LinkT, toplevel: bool = True) -> LinkT | None:
        raise NotImplementedError()

    def add_node(self, node_params: NodeT) -> NodeT | None:
        raise NotImplementedError()

    def get_incoming_links_handles(self, atom_handle: str, **kwargs) -> IncomingLinksT:
        links = self.incoming_set.get(atom_handle) or []
        return links

    def get_incoming_links_atoms(self, atom_handle: str, **kwargs) -> IncomingLinksT:
        links = self.incoming_set.get(atom_handle) or []
        return [self._get_atom(link) for link in links]

    def get_atom_type(self, handle: str) -> str | None:
        raise NotImplementedError()

    def reindex(
        self, pattern_index_templates: dict[str, list[dict[str, Any]]] | None = None
    ) -> None:
        raise NotImplementedError()

    def delete_atom(self, handle: str, **kwargs) -> None:
        raise NotImplementedError()

    def create_field_index(
        self,
        atom_type: str,
        fields: list[str],
        named_type: str | None = None,
        composite_type: list[Any] | None = None,
        index_type: FieldIndexType | None = None,
    ) -> str:
        raise NotImplementedError()

    def bulk_insert(self, documents: list[AtomT]) -> None:
        raise NotImplementedError()

    def retrieve_all_atoms(self) -> list[AtomT]:
        raise NotImplementedError()

    def get_node_by_name(self, node_type: str, substring: str) -> list[str]:
        return self._get_matched_node_name(node_type, substring)

    def get_atoms_by_field(self, query: list[OrderedDict[str, str]]) -> list[str]:
        answer = []

        def _append_atom(atom, named_type, name):
            for q in query:
                if q["field"] == named_type and q["value"] in name:
                    return True
            return False

        for atom in list(self.all_nodes.keys()) + self.all_links:
            if isinstance(atom, str):
                named_type, name = _split_node_handle(atom)
                if _append_atom(atom, named_type, name):
                    answer.append(atom)
            else:
                for a in atom[1:]:
                    named_type, name = _split_node_handle(a)
                    if _append_atom(atom, named_type, name):
                        answer.append(atom)
                        break
        return answer

    def get_atoms_by_index(
        self,
        index_id: str,
        query: list[OrderedDict[str, str]],
        cursor: int = 0,
        chunk_size: int = 500,
    ) -> tuple[int, list[AtomT]]:
        raise NotImplementedError()

    def get_atoms_by_text_field(
        self,
        text_value: str,
        field: str | None = None,
        text_index_id: str | None = None,
    ) -> list[str]:
        answer = []
        for atom in list(self.all_nodes.keys()) + self.all_links:
            if isinstance(atom, str):
                named_type, name = _split_node_handle(atom)
                if text_value in name:
                    answer.append(atom)
            else:
                for a in atom[1:]:
                    named_type, name = _split_node_handle(a)
                    if text_value in name:
                        answer.append(atom)
                        break
        return answer

    def get_node_by_name_starting_with(self, node_type: str, startswith: str) -> list[str]:
        answer = []
        if node_type == self.node_type:
            for node in self.all_nodes:
                _, name = _split_node_handle(node)
                if name.startswith(startswith):
                    answer.append(node)
        return answer

    def commit(self, **kwargs) -> None:
        raise NotImplementedError()


class DatabaseAnimals(DatabaseMock):
    def __init__(self):
        super().__init__()

        self.all_links = [
            ["Similarity", self.human, self.monkey],
            ["Similarity", self.human, self.chimp],
            ["Similarity", self.chimp, self.monkey],
            ["Similarity", self.snake, self.earthworm],
            ["Similarity", self.rhino, self.triceratops],
            ["Similarity", self.snake, self.vine],
            ["Similarity", self.human, self.ent],
            ["Inheritance", self.human, self.mammal],
            ["Inheritance", self.monkey, self.mammal],
            ["Inheritance", self.chimp, self.mammal],
            ["Inheritance", self.mammal, self.animal],
            ["Inheritance", self.reptile, self.animal],
            ["Inheritance", self.snake, self.reptile],
            ["Inheritance", self.dinosaur, self.reptile],
            ["Inheritance", self.triceratops, self.dinosaur],
            ["Inheritance", self.earthworm, self.animal],
            ["Inheritance", self.rhino, self.mammal],
            ["Inheritance", self.vine, self.plant],
            ["Inheritance", self.ent, self.plant],
            ["Similarity", self.monkey, self.human],
            ["Similarity", self.chimp, self.human],
            ["Similarity", self.monkey, self.chimp],
            ["Similarity", self.earthworm, self.snake],
            ["Similarity", self.triceratops, self.rhino],
            ["Similarity", self.vine, self.snake],
            ["Similarity", self.ent, self.human],
        ]

        self.incoming_set = {}

        for link in self.all_links:
            self._add_incoming_set(str(link), link[1:])

    def add_link(self, link_params: LinkT, toplevel: bool = True) -> LinkT | None:
        if link_params in self.all_links:
            index = self.all_links.index(link_params)
            self.all_links[index] = link_params
        else:
            self.all_links.append(link_params)
        return link_params
