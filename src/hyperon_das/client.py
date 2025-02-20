import contextlib
import pickle
from typing import Any, Dict, Iterator, List, Optional, Tuple, Union

from hyperon_das_atomdb import AtomDoesNotExist
from hyperon_das_atomdb.database import AtomT, IncomingLinksT
from requests import exceptions, sessions

from hyperon_das.exceptions import (
    FunctionsConnectionError,
    FunctionsTimeoutError,
    HTTPError,
    RequestError,
)
from hyperon_das.link_filters import LinkFilter
from hyperon_das.logger import logger
from hyperon_das.type_alias import Query
from hyperon_das.utils import connect_to_server, das_error, deserialize, serialize


class FunctionsClient:
    def __init__(self, host: str, port: int, name: Optional[str] = None) -> None:
        if not host and not port:
            das_error(ValueError("'host' and 'port' are mandatory parameters"))
        self.name = name if name else f"client_{host}:{port}"
        self.status_code, self.url = connect_to_server(host, port)

    def _send_request(self, payload) -> Any:
        try:
            if payload.get("input"):
                normalized_input = {k: v for k, v in payload["input"].items() if v is not None}
                payload["input"] = normalized_input

            payload_serialized = serialize(payload)

            with sessions.Session() as session:
                response = session.request(
                    method="POST",
                    url=self.url,
                    data=payload_serialized,
                    headers={"Content-Type": "application/octet-stream"},
                )

            response.raise_for_status()

            try:
                response_data = deserialize(response.content)
            except pickle.UnpicklingError as e:
                das_error(Exception(f"Unpickling error: {str(e)}"))

            if response.status_code == 200:
                return response_data
            else:
                return response_data.get(
                    "error", f"Unknown error with status code {response.status_code}"
                )
        except exceptions.ConnectionError as e:
            das_error(
                FunctionsConnectionError(
                    message=f"Connection error for URL: '{self.url}' with payload: '{payload}'",
                    details=str(e),
                )
            )
        except exceptions.Timeout as e:
            das_error(
                FunctionsTimeoutError(
                    message=f"Request timed out for URL: '{self.url}' with payload: '{payload}'",
                    details=str(e),
                )
            )
        except exceptions.HTTPError as e:
            with contextlib.suppress(pickle.UnpicklingError):
                message = deserialize(response.content)
                das_error(
                    HTTPError(
                        message="Please, check if your request payload is correctly formatted.",
                        details=message,
                        status_code=e.response.status_code,
                    )
                )
        except exceptions.RequestException as e:
            das_error(
                RequestError(
                    message=f"Request exception for URL: '{self.url}' with payload: '{payload}'.",
                    details=str(e),
                )
            )

    def get_atom(self, handle: str, **kwargs) -> Union[str, Dict]:
        payload = {
            "action": "get_atom",
            "input": {"handle": handle},
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            if e.status_code == 404:
                raise AtomDoesNotExist("Nonexistent atom") from e
            else:
                raise e

    def get_links(self, link_filter: LinkFilter) -> Union[List[str], List[Dict]]:
        payload = {
            "action": "get_links",
            "input": {
                "link_filter": {
                    "filter_type": link_filter.filter_type,
                    "toplevel_only": link_filter.toplevel_only,
                    "link_type": link_filter.link_type or "",
                    "target_types": link_filter.target_types or [],
                    "targets": link_filter.targets or [],
                }
            },
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            if e.status_code == 404:
                raise AtomDoesNotExist("Nonexistent atom") from e
            elif e.status_code == 400:
                raise ValueError(str(e)) from e
            else:
                raise e

    def query(
        self,
        query: Dict[str, Any],
        parameters: Optional[Dict[str, Any]] = None,
    ) -> List[AtomT]:
        try:
            payload = {
                "action": "query",
                "input": {"query": query, "parameters": parameters},
            }
            return self._send_request(payload)
        except HTTPError as e:
            if e.status_code == 400:
                raise ValueError(
                    "Your query couldn't be processed due to an invalid format. Review the way the query "
                    "is written and try again.",
                    str(e),
                ) from e
            elif e.status_code == 404:
                raise Exception(
                    "Your query couldn't be processed because Atom nonexistent", str(e)
                ) from e
            raise e

    def count_atoms(self, parameters: Optional[Dict[str, Any]] = None) -> Dict[str, int]:
        payload = {
            "action": "count_atoms",
            "input": {"parameters": parameters},
        }
        return self._send_request(payload)

    def commit_changes(self, **kwargs) -> Tuple[int, int]:
        payload = {
            "action": "commit_changes",
            "input": {"kwargs": kwargs},
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            if e.status_code == 403:
                raise ValueError(str(e)) from e
            else:
                raise e

    def get_incoming_links(self, atom_handle: str, **kwargs) -> IncomingLinksT | Iterator:
        payload = {
            "action": "get_incoming_links",
            "input": {"atom_handle": atom_handle, "kwargs": kwargs},
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            logger().debug(f"Error during `get_incoming_links` request on remote Das: {str(e)}")
            return []

    def create_field_index(
        self,
        atom_type: str,
        fields: List[str],
        named_type: Optional[str] = None,
        composite_type: Optional[List[Any]] = None,
        index_type: Optional[str] = None,
    ) -> str:
        payload = {
            "action": "create_field_index",
            "input": {
                "atom_type": atom_type,
                "fields": fields,
                "named_type": named_type,
                "composite_type": composite_type,
                "index_type": index_type,
            },
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            if e.status_code == 400:
                raise ValueError(str(e)) from e
            else:
                raise e

    def custom_query(self, index_id: str, query: Query, **kwargs) -> List[AtomT]:
        payload = {
            "action": "custom_query",
            "input": {
                "index_id": index_id,
                "query": {v["field"]: v["value"] for v in query},
                "kwargs": kwargs,
            },
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            raise e

    def fetch(
        self,
        query: Union[List[dict], dict],
        host: Optional[str] = None,
        port: Optional[int] = None,
        **kwargs,
    ) -> Any:
        payload = {
            "action": "fetch",
            "input": {"query": query, "host": host, "port": port, "kwargs": kwargs},
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            raise e

    def create_context(self, name: str, queries: Optional[List[Query]]) -> Any:
        payload = {
            "action": "create_context",
            "input": {"name": name, "queries": queries},
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            if e.status_code == 404:
                raise AtomDoesNotExist("nonexistent atom") from e
            elif e.status_code == 400:
                raise ValueError(str(e)) from e
            else:
                raise e

    def get_atoms_by_field(self, query: Query) -> List[str]:
        payload = {
            "action": "get_atoms_by_field",
            "input": {"query": {v["field"]: v["value"] for v in query}},
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            if e.status_code == 400:
                raise ValueError(str(e)) from e
            else:
                raise e

    def get_atoms_by_text_field(
        self,
        text_value: str,
        field: Optional[str] = None,
        text_index_id: Optional[str] = None,
    ) -> List[str]:
        payload = {
            "action": "get_atoms_by_text_field",
            "input": {
                "text_value": text_value,
                "field": field,
                "text_index_id": text_index_id,
            },
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            if e.status_code == 400:
                raise ValueError(str(e)) from e
            else:
                raise e

    def get_node_by_name_starting_with(self, node_type: str, startswith: str) -> List[str]:
        payload = {
            "action": "get_node_by_name_starting_with",
            "input": {
                "node_type": node_type,
                "startswith": startswith,
            },
        }
        try:
            return self._send_request(payload)
        except HTTPError as e:
            if e.status_code == 400:
                raise ValueError(str(e)) from e
            else:
                raise e
