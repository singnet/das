from collections import OrderedDict
from typing import Any, Dict, Iterator, List, Optional, Set, Union

from hyperon_das_atomdb import WILDCARD, AtomDB
from hyperon_das_atomdb.adapters import InMemoryDB
from hyperon_das_atomdb.database import (
    AtomT,
    HandleListT,
    HandleSetT,
    HandleT,
    IncomingLinksT,
    LinkT,
    NodeT,
)
from hyperon_das_atomdb.exceptions import AtomDoesNotExist

from hyperon_das.cache.cache_controller import CacheController
from hyperon_das.cache.iterators import (
    AndEvaluator,
    CustomQuery,
    LazyQueryEvaluator,
    ListIterator,
    QueryAnswerIterator,
)
from hyperon_das.client import FunctionsClient
from hyperon_das.context import Context
from hyperon_das.exceptions import UnexpectedQueryFormat
from hyperon_das.link_filters import LinkFilter, LinkFilterType
from hyperon_das.logger import logger
from hyperon_das.query_engines.query_engine_protocol import QueryEngine
from hyperon_das.type_alias import Query
from hyperon_das.utils import Assignment, QueryAnswer, das_error


class LocalQueryEngine(QueryEngine):
    def __init__(
        self,
        backend: AtomDB,
        cache_controller: CacheController,
        system_parameters: Dict[str, Any],
        **kwargs,
    ) -> None:
        self.system_parameters = system_parameters
        self.local_backend = backend
        self.buffer: list[dict[str, Any]] = []
        self.cache_controller = cache_controller

    def _recursive_query(
        self,
        query: Query,
        mappings: Set[Assignment] | None = None,  # TODO: this parameter is never used
        parameters: dict[str, Any] | None = None,
    ) -> QueryAnswerIterator:
        if isinstance(query, list):
            sub_expression_results = [
                self._recursive_query(expression, mappings, parameters) for expression in query
            ]
            return AndEvaluator(sub_expression_results)
        elif query["atom_type"] == "node":
            try:
                atom_handle = self.local_backend.get_node_handle(query["type"], query["name"])
                return ListIterator([QueryAnswer(self.local_backend.get_atom(atom_handle), None)])
            except AtomDoesNotExist:
                return ListIterator([])
        elif query["atom_type"] == "link":
            matched_targets = []
            for target in query["targets"]:
                if target["atom_type"] == "node" or target["atom_type"] == "link":
                    matched = self._recursive_query(target, mappings, parameters)
                    if matched:
                        matched_targets.append(matched)
                elif target["atom_type"] == "variable":
                    matched_targets.append(ListIterator([QueryAnswer(target, None)]))
                else:
                    das_error(
                        UnexpectedQueryFormat(
                            message="Query processing reached an unexpected state",
                            details=f"link: {str(query)} link target: {str(query)}",
                        )
                    )
            return LazyQueryEvaluator(
                link_type=query["type"], source=matched_targets, query_engine=self
            )
        else:
            das_error(
                UnexpectedQueryFormat(
                    message="Query processing reached an unexpected state",
                    details=f"query: {str(query)}",
                )
            )

    def _get_related_links(
        self,
        link_type: str,
        target_types: list[str] | None = None,
        link_targets: HandleListT | None = None,
        **kwargs,
    ) -> HandleSetT:
        if link_type != WILDCARD and target_types is not None:
            return self.local_backend.get_matched_type_template(
                [link_type, *target_types], **kwargs
            )
        elif link_targets is not None:
            try:
                return self.local_backend.get_matched_links(link_type, link_targets, **kwargs)
            except AtomDoesNotExist:
                return set()
        elif link_type != WILDCARD:
            return self.local_backend.get_all_links(link_type, **kwargs)
        else:
            das_error(
                ValueError(
                    f"Invalid parameters. link_type = {link_type} target_types = {target_types} link_targets = {link_targets}"
                )
            )

    def _process_node(self, query: dict) -> List[NodeT]:
        try:
            handle = self.local_backend.node_handle(query["type"], query["name"])
            return [self.local_backend.get_atom(handle, no_target_format=True)]
        except AtomDoesNotExist:
            return []

    def _process_link(self, query: dict) -> List[LinkT]:
        target_handles = self._generate_target_handles(query["targets"])
        matched_links = self.local_backend.get_matched_links(
            link_type=query["type"], target_handles=target_handles
        )
        unique_handles = set()
        result = []

        for link_handle in matched_links:
            link_targets = self.get_atom(link_handle).targets

            if link_handle not in unique_handles:
                unique_handles.add(link_handle)
                result.append(self.local_backend.get_atom(link_handle, no_target_format=True))

            for target in link_targets:
                atoms = self._handle_to_atoms(target)
                if isinstance(atoms, list):
                    for atom in atoms:
                        if atom.id not in unique_handles:
                            unique_handles.add(atom.id)
                            result.append(atom)
                else:
                    if atoms.id not in unique_handles:
                        unique_handles.add(atoms.id)
                        result.append(atoms)

        return result

    def _generate_target_handles(
        self, targets: List[Dict[str, Any]]
    ) -> list[HandleT | HandleListT | list[Any]]:  # multiple levels of nested lists due to
        # recursion
        targets_hash: list[HandleT | HandleListT | list[Any]] = []
        for target in targets:
            handle: HandleT | HandleListT | None = None
            if target["atom_type"] == "node":
                handle = self.local_backend.node_handle(target["type"], target["name"])
            elif target["atom_type"] == "link":
                handle = self._generate_target_handles(target["targets"])
            elif target["atom_type"] == "variable":
                handle = WILDCARD
            targets_hash.append(handle)
        return targets_hash

    def _handle_to_atoms(self, handle: str) -> Union[List[AtomT], AtomT]:
        try:
            atom = self.local_backend.get_atom(handle, no_target_format=True)
        except AtomDoesNotExist:
            return []

        if isinstance(atom, NodeT):
            return atom
        else:  # link
            answer = [atom]
            for target in atom.targets:
                answer.append(self._handle_to_atoms(target))
            return answer

    def has_buffer(self) -> bool:
        if isinstance(self.local_backend, InMemoryDB):
            atoms = self.local_backend.db.node
            atoms.update(self.local_backend.db.link)
            self.buffer = list(atoms.values())
            return bool(atoms)
        return False

    def get_atom(self, handle: str, **kwargs) -> AtomT:
        try:
            return self.local_backend.get_atom(handle, **kwargs)
        except AtomDoesNotExist as exception:
            das_error(exception)

    def get_atoms(self, handles: str, **kwargs) -> List[AtomT]:
        return [self.local_backend.get_atom(handle, **kwargs) for handle in handles]

    def get_link_handles(self, link_filter: LinkFilter) -> HandleSetT:
        if link_filter.filter_type == LinkFilterType.FLAT_TYPE_TEMPLATE:
            return self.local_backend.get_matched_type_template(
                [link_filter.link_type, *link_filter.target_types],
                toplevel_only=link_filter.toplevel_only,
            )
        elif link_filter.filter_type == LinkFilterType.TARGETS:
            return self.local_backend.get_matched_links(
                link_filter.link_type,
                link_filter.targets,
                toplevel_only=link_filter.toplevel_only,
            )
        elif link_filter.filter_type == LinkFilterType.NAMED_TYPE:
            return self.local_backend.get_all_links(
                link_filter.link_type, toplevel_only=link_filter.toplevel_only
            )
        else:
            das_error(ValueError("Invalid LinkFilterType: {link_filter.filter_type}"))

    def get_links(self, link_filter: LinkFilter) -> List[LinkT]:
        handles = self.get_link_handles(link_filter)
        if handles:
            return [self.get_atom(handle) for handle in handles]
        else:
            return []

    def get_incoming_links(self, atom_handle: str, **kwargs) -> IncomingLinksT:
        if kwargs.get("handles_only", False):
            return self.local_backend.get_incoming_links_handles(atom_handle, **kwargs)
        return self.local_backend.get_incoming_links_atoms(atom_handle, **kwargs)

    def query(
        self,
        query: Query,
        parameters: dict[str, Any] | None = None,
    ) -> Iterator[QueryAnswer] | list[AtomT]:
        no_iterator = parameters.get("no_iterator", False) if parameters else False
        if no_iterator:
            logger().debug(
                {
                    "message": "[DistributedAtomSpace][query] - Start",
                    "data": {"query": query, "parameters": parameters},
                }
            )
        query_results = self._recursive_query(query, parameters)
        if no_iterator:
            answer = list(query_results)
            logger().debug(f"query: {query} result: {str(answer)}")
            return answer
        return query_results

    def custom_query(
        self, index_id: str, query: list[OrderedDict[str, str]], **kwargs
    ) -> Iterator | tuple[int, list[AtomT]]:
        if kwargs.pop("no_iterator", True):
            return self.local_backend.get_atoms_by_index(index_id, query=query, **kwargs)
        else:
            if kwargs.get("cursor") is None:
                kwargs["cursor"] = 0
            cursor, answer = self.local_backend.get_atoms_by_index(index_id, query=query, **kwargs)
            kwargs["backend"] = self.local_backend
            kwargs["index_id"] = index_id
            kwargs["cursor"] = cursor
            return CustomQuery(ListIterator(answer), **kwargs)

    def count_atoms(self, parameters: Optional[Dict[str, Any]] = None) -> Dict[str, int]:
        if parameters and parameters.get("context") == "remote":
            return {}
        return self.local_backend.count_atoms(parameters)

    def commit(self, **kwargs) -> None:
        if kwargs.get("buffer"):
            self.local_backend.commit(buffer=kwargs["buffer"])
        self.local_backend.commit()

    def reindex(self, pattern_index_templates: Optional[Dict[str, Dict[str, Any]]] = None):
        self.local_backend.reindex(pattern_index_templates)

    def create_field_index(
        self,
        atom_type: str,
        fields: List[str],
        named_type: Optional[str] = None,
        composite_type: Optional[List[Any]] = None,
        index_type: Optional[str] = None,
    ) -> str:
        return self.local_backend.create_field_index(
            atom_type, fields, named_type, composite_type, index_type
        )

    def fetch(
        self,
        query: Query,
        host: Optional[str] = None,
        port: Optional[int] = None,
        **kwargs,
    ) -> Any:
        if not self.system_parameters.get("running_on_server"):  # Local
            if host is not None and port is not None:
                server = FunctionsClient(host, port)
            else:
                server = self.local_backend
            return server.fetch(query=query, **kwargs)
        else:
            if query is None:
                try:
                    return self.local_backend.retrieve_all_atoms()
                except Exception as e:
                    das_error(e)
            else:
                if "atom_type" not in query:
                    das_error(ValueError("Invalid query: missing atom_type"))

                atom_type = query["atom_type"]

                if atom_type == "node":
                    return self._process_node(query)
                elif atom_type == "link":
                    return self._process_link(query)
                else:
                    das_error(
                        ValueError("Invalid atom type: {atom_type}. Use 'node' or 'link' instead.")
                    )

    def create_context(
        self,
        name: str,
        queries: list[Query] | None = None,
    ) -> Context:  # type: ignore
        das_error(NotImplementedError("Contexts are not implemented for non-server local DAS"))

    def get_atoms_by_field(self, query: list[OrderedDict[str, str]]) -> HandleListT:
        return self.local_backend.get_atoms_by_field(query)

    def get_atoms_by_text_field(
        self,
        text_value: str,
        field: Optional[str] = None,
        text_index_id: Optional[str] = None,
    ) -> HandleListT:
        return self.local_backend.get_atoms_by_text_field(text_value, field, text_index_id)

    def get_node_by_name_starting_with(self, node_type: str, startswith: str) -> HandleListT:
        return self.local_backend.get_node_by_name_starting_with(node_type, startswith)
