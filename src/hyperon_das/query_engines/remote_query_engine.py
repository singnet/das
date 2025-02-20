from enum import Enum
from typing import Any, Dict, Iterator, List, Optional

from hyperon_das_atomdb.database import (
    AtomT,
    HandleListT,
    HandleSetT,
    HandleT,
    IncomingLinksT,
    LinkT,
)
from hyperon_das_atomdb.exceptions import AtomDoesNotExist

from hyperon_das.cache.cache_controller import CacheController
from hyperon_das.cache.iterators import CustomQuery, ListIterator
from hyperon_das.client import FunctionsClient
from hyperon_das.context import Context
from hyperon_das.exceptions import InvalidDASParameters, QueryParametersException
from hyperon_das.link_filters import LinkFilter
from hyperon_das.query_engines.local_query_engine import LocalQueryEngine
from hyperon_das.query_engines.query_engine_protocol import QueryEngine
from hyperon_das.type_alias import Query
from hyperon_das.utils import QueryAnswer, das_error


class QueryScopes(Enum):
    REMOTE_ONLY = "remote_only"
    SYNCHRONOUS_UPDATE = "synchronous_update"
    LOCAL_ONLY = "local_only"
    LOCAL_AND_REMOTE = "local_and_remote"


class RemoteQueryEngine(QueryEngine):
    def __init__(
        self,
        backend,
        cache_controller: CacheController,
        system_parameters: Dict[str, Any],
        **kwargs,
    ):
        self.system_parameters = system_parameters
        self.local_query_engine = LocalQueryEngine(backend, cache_controller, kwargs)
        self.cache_controller = cache_controller
        self.__mode = kwargs.get("mode", "read-only")
        self.host = kwargs.get("host")
        self.port = kwargs.get("port")
        if not self.host or not self.port:
            das_error(InvalidDASParameters(message="'host' and 'port' are mandatory parameters"))
        self.remote_das = FunctionsClient(self.host, self.port)
        self.query_scope_values = {*[q.value for q in QueryScopes]}

    @property
    def mode(self):
        return self.__mode

    def get_atom(self, handle: HandleT, **kwargs) -> AtomT:
        atom = self.cache_controller.get_atom(handle)
        if atom is None:
            try:
                atom = self.local_query_engine.get_atom(handle, **kwargs)
            except AtomDoesNotExist:
                try:
                    atom = self.remote_das.get_atom(handle, **kwargs)
                except AtomDoesNotExist as exception:
                    das_error(exception)
        return atom

    def get_atoms(self, handles: HandleListT, **kwargs) -> List[AtomT]:
        return self.cache_controller.get_atoms(handles)

    def get_links(self, link_filter: LinkFilter) -> List[LinkT]:
        links = self.local_query_engine.get_links(link_filter)
        remote_links = self.remote_das.get_links(link_filter)
        links.extend(remote_links)
        return links

    def get_link_handles(self, link_filter: LinkFilter) -> HandleSetT:
        # TODO Implement get_link_handles() in faas client
        return {link["handle"] for link in self.get_links(link_filter)}

    def get_incoming_links(self, atom_handle: HandleT, **kwargs) -> IncomingLinksT:
        links = self.local_query_engine.get_incoming_links(atom_handle, **kwargs)
        remote_links = self.remote_das.get_incoming_links(atom_handle, **kwargs)
        links.extend(remote_links)
        return links

    def custom_query(self, index_id: str, query: Query, **kwargs) -> Iterator:
        kwargs.pop("no_iterator", None)
        if kwargs.get("cursor") is None:
            kwargs["cursor"] = 0
        answer = self.remote_das.custom_query(index_id, query=query, **kwargs)
        kwargs["backend"] = self.remote_das
        kwargs["index_id"] = index_id
        kwargs["is_remote"] = True
        return CustomQuery(ListIterator(answer), **kwargs)

    def query(
        self,
        query: Query,
        parameters: Dict[str, Any] | None = None,
    ) -> Iterator[QueryAnswer] | list[dict[str, Any]]:
        parameters = parameters or {}
        query_scope = parameters.get("query_scope", "remote_only")
        try:
            query_scope = QueryScopes(query_scope)
        except ValueError:
            das_error(
                QueryParametersException(
                    message=f'Invalid value for "query_scope": "{query_scope}"'
                )
            )

        if query_scope == QueryScopes.LOCAL_AND_REMOTE:
            das_error(
                QueryParametersException(
                    message=(
                        f"Invalid value for parameter 'query_scope': '{query_scope}'. "
                        "This type of query scope is not implemented yet"
                    )
                )
            )

        if query_scope in {QueryScopes.REMOTE_ONLY, QueryScopes.SYNCHRONOUS_UPDATE}:
            if query_scope == QueryScopes.SYNCHRONOUS_UPDATE:
                self.commit()
            parameters["no_iterator"] = True
            return self.remote_das.query(query, parameters)

        return self.local_query_engine.query(query, parameters)

    def count_atoms(self, parameters: Optional[Dict[str, Any]] = None) -> Dict[str, int]:
        if (context := parameters.get("context") if parameters else None) == "local":
            return self.local_query_engine.count_atoms(parameters)
        if context == "remote":
            return self.remote_das.count_atoms(parameters)

        local_answer = self.local_query_engine.count_atoms(parameters)
        remote_answer = self.remote_das.count_atoms(parameters)
        return {
            k: (local_answer.get(k, 0) + remote_answer.get(k, 0))
            for k in ["node_count", "link_count", "atom_count"]
        }

    #
    def commit(self, **kwargs) -> None:
        if self.__mode == "read-write":
            if self.local_query_engine.has_buffer():
                self.remote_das.commit_changes(buffer=self.local_query_engine.buffer)
            self.remote_das.commit_changes()
        elif self.__mode == "read-only":
            das_error(PermissionError("Commit can't be executed in read mode"))
        else:
            das_error(ValueError("Invalid mode: '{self.__mode}'. Use 'read-only' or 'read-write'"))

    def reindex(self, pattern_index_templates: Optional[Dict[str, Dict[str, Any]]]):
        das_error(NotImplementedError())

    def create_field_index(
        self,
        atom_type: str,
        fields: List[str],
        named_type: Optional[str] = None,
        composite_type: Optional[List[Any]] = None,
        index_type: Optional[str] = None,
    ) -> str:
        return self.remote_das.create_field_index(
            atom_type, fields, named_type, composite_type, index_type
        )

    def fetch(
        self,
        query: Query,
        host: Optional[str] = None,
        port: Optional[int] = None,
        **kwargs,
    ) -> Any:
        if host is not None and port is not None:
            server = FunctionsClient(host, port)
        else:
            server = self.remote_das
        return server.fetch(query=query, **kwargs)

    def create_context(
        self,
        name: str,
        queries: Optional[List[Query]] = None,
    ) -> Context:
        return self.remote_das.create_context(name, queries)

    def get_atoms_by_field(self, query: Query) -> HandleListT:
        return self.remote_das.get_atoms_by_field(query)

    def get_atoms_by_text_field(
        self,
        text_value: str,
        field: Optional[str] = None,
        text_index_id: Optional[str] = None,
    ) -> HandleListT:
        return self.remote_das.get_atoms_by_text_field(text_value, field, text_index_id)

    def get_node_by_name_starting_with(self, node_type: str, startswith: str) -> HandleListT:
        return self.remote_das.get_node_by_name_starting_with(node_type, startswith)
