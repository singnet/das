import pickle
import re
from dataclasses import dataclass
from http import HTTPStatus  # noqa: F401
from importlib import import_module
from typing import Any, Dict, FrozenSet, List, Optional, Set, Tuple, Union

from requests import sessions
from requests.exceptions import (  # noqa: F401
    ConnectionError,
    HTTPError,
    JSONDecodeError,
    RequestException,
    Timeout,
)

from hyperon_das.decorators import retry
from hyperon_das.exceptions import InvalidAssignment
from hyperon_das.logger import logger


def das_error(exception: Exception):
    logger().error(str(exception))
    raise exception


class Assignment:
    @staticmethod
    def compose(components: List["Assignment"]) -> Optional["Assignment"]:
        answer = Assignment()
        for component in components:
            if not answer.merge(component):
                return None
        answer.freeze()
        return answer

    def __init__(self, other: Optional["Assignment"] = None):
        self.labels: Union[Set[str], FrozenSet] = set()
        self.values: Union[Set[str], FrozenSet] = set()
        self.hashcode: int = 0
        if other:
            self.mapping: Dict[str, str] = dict(other.mapping)
        else:
            self.mapping: Dict[str, str] = {}

    def __hash__(self) -> int:
        assert self.hashcode
        return self.hashcode

    def __eq__(self, other) -> bool:
        assert self.hashcode and other.hashcode
        return self.hashcode == other.hashcode

    def __lt__(self, other) -> bool:
        assert self.hashcode and other.hashcode
        return self.hashcode < other.hashcode

    def __repr__(self) -> str:
        return str([tuple([label, self.mapping[label]]) for label in sorted(self.labels)])

    def __str__(self) -> str:
        return self.__repr__()

    def frozen(self):
        return self.hashcode != 0

    def freeze(self) -> bool:
        if self.frozen():
            return False
        else:
            self.labels = frozenset(self.labels)
            self.values = frozenset(self.values)
            self.hashcode = hash(frozenset(self.mapping.items()))
            return True

    def assign(
        self,
        label: str,
        value: str,
        parameters: Optional[Dict[str, Any]] = None,
    ) -> bool:
        if label is None or value is None or self.frozen():
            raise InvalidAssignment(
                message="Invalid assignment",
                details=f"label = {label} value = {value} hashcode = {self.hashcode}",
            )
        if label in self.labels:
            return self.mapping[label] == value
        else:
            if parameters and parameters["no_overload"] and value in self.values:
                return False
            self.labels.add(label)
            self.values.add(value)
            self.mapping[label] = value
            return True

    def merge(self, other: "Assignment", in_place: bool = True) -> Optional[bool]:
        if in_place:
            assert not self.frozen()
            if other:
                for label, value in other.mapping.items():
                    if not self.assign(label, value):
                        return False
            return True
        else:
            new_assignment = Assignment(self)
            if new_assignment.merge(other):
                new_assignment.freeze()
                return new_assignment
            else:
                return None


@dataclass
class QueryAnswer:
    subgraph: Optional[Dict] = None
    assignment: Optional[Assignment] = None

    def _recursive_get_handle_set(self, atom, handle_set):
        handle_set.add(atom["handle"])
        targets = atom.get("targets", None)
        if targets is not None:
            for target_atom in targets:
                self._recursive_get_handle_set(target_atom, handle_set)

    def _recursive_get_handle_count(self, atom, handle_count):
        key = atom["handle"]
        total = handle_count.get(key, 0) + 1
        handle_count[key] = total
        targets = atom.get("targets", None)
        if targets is not None:
            for target_atom in targets:
                self._recursive_get_handle_count(target_atom, handle_count)

    def get_handle_set(self):
        handle_set = set()
        if self.subgraph is not None:
            self._recursive_get_handle_set(self.subgraph, handle_set)
        return handle_set

    def get_handle_count(self):
        handle_count = {}
        if self.subgraph is not None:
            self._recursive_get_handle_count(self.subgraph, handle_count)
        return handle_count


def get_package_version(package_name: str) -> str:
    package_module = import_module(package_name)
    return getattr(package_module, "__version__", None)


def serialize(payload: Any) -> bytes:
    return pickle.dumps(payload)


def deserialize(payload: bytes) -> Any:
    return pickle.loads(payload)


@retry(attempts=5, timeout_seconds=120)
def connect_to_server(host: str, port: int) -> Tuple[int, str]:
    """Connect to the server and return the status connection and the url server"""
    port = port or "8081"
    openfaas_uri = f"http://{host}:{port}/function/query-engine"
    aws_lambda_uri = f"http://{host}/prod/query-engine"

    for uri in [openfaas_uri, aws_lambda_uri]:
        status_code, message = check_server_connection(uri)
        if status_code == HTTPStatus.OK:
            break
        elif status_code == HTTPStatus.INTERNAL_SERVER_ERROR:
            raise Exception(message)

    return status_code, uri


def check_server_connection(url: str) -> Tuple[int, str]:
    logger().debug(f"Connecting to remote DAS {url}")

    try:
        with sessions.Session() as session:
            payload = {
                "action": "handshake",
                "input": {},
            }
            response = session.post(
                url=url,
                data=serialize(payload),
                headers={"Content-Type": "application/octet-stream"},
                timeout=10,
            )

        response.raise_for_status()

        check_versions(response.content)

        return response.status_code, "Successful connection"

    except pickle.UnpicklingError:
        logger().error("Failed to unpickle response from server.")
        return 500, "Error unpickling objects in server response"
    except (ConnectionError, Timeout, HTTPError, RequestException) as e:
        logger().error(f"Connection error: {str(e)}")
        return 400, f"Connection failed: {str(e)}"
    except Exception as e:
        msg = f"Unexpected error: {str(e)}"
        logger().error(msg)
        return 500, str(msg)


def check_versions(response_body: bytes) -> None:
    """
    Checks the versions of local packages and the ones used by the remote server, and compare them
    to ensure they are using compatible versions.

    Parameters:
        response_body (bytes): response body from remote server containing the packages versions.

    Returns:
        None

    Raises:
        ValueError: If the content is not in the expected format or
                    if any dependency version does not meet the criteria.
    """
    remote_data = deserialize(response_body)
    remote_das_version = remote_data.get("das", {}).get("version")
    remote_atomdb_version = remote_data.get("atom_db", {}).get("version")

    if not remote_das_version or not remote_atomdb_version:
        raise ValueError("Invalid response from server, missing version info.")

    # local packages versions
    das_version = get_package_version("hyperon_das")
    atom_db_version = get_package_version("hyperon_das_atomdb")

    if not das_version or not atom_db_version:
        raise ValueError("Missing version info in the local packages.")

    is_atomdb_compatible = compare_minor_versions(
        remote_atomdb_version,
        atom_db_version,
    )
    is_das_compatible = compare_minor_versions(
        remote_das_version,
        das_version,
    )

    if not is_atomdb_compatible or not is_das_compatible:
        local_versions = f"hyperon-das: {das_version}, hyperon-das-atomdb: {atom_db_version}"
        remote_versions = (
            f"hyperon-das: {remote_das_version}, hyperon-das-atomdb: {remote_atomdb_version}"
        )
        error_message = f"Version mismatch. Local: {local_versions}. Remote: {remote_versions}."
        logger().error(error_message)
        raise Exception(error_message)


def get_version_components(version_string: str) -> Union[Tuple[int, int, int], None]:
    pattern = r"^(\d+)\.(\d+)\.(\d+)$"
    match = re.match(pattern, version_string)

    if match:
        return tuple(map(int, match.groups()))

    return None


def compare_versions(version1: str, version2: str, component_index: int) -> Union[int, None]:
    components1 = get_version_components(version1)
    components2 = get_version_components(version2)

    if components1 is None or components2 is None:
        return None

    for i in range(component_index + 1):
        if components1[i] != components2[i]:
            return False

    return True


def compare_major_versions(version1: str, version2: str) -> Union[int, None]:
    return compare_versions(version1, version2, 0)


def compare_minor_versions(version1: str, version2: str) -> Union[int, None]:
    return compare_versions(version1, version2, 1)


def compare_patch_versions(version1: str, version2: str) -> Union[int, None]:
    return compare_versions(version1, version2, 2)
