from typing import ClassVar as _ClassVar
from typing import Iterable as _Iterable
from typing import Mapping as _Mapping
from typing import Optional as _Optional

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf.internal import containers as _containers

DESCRIPTOR: _descriptor.FileDescriptor

class HandleList(_message.Message):
    __slots__ = ("list", "request_type", "hebbian_network", "context")

    LIST_FIELD_NUMBER: _ClassVar[int]
    REQUEST_TYPE_FIELD_NUMBER: _ClassVar[int]
    HEBBIAN_NETWORK_FIELD_NUMBER: _ClassVar[int]
    CONTEXT_FIELD_NUMBER: _ClassVar[int]

    list: _containers.RepeatedScalarFieldContainer[str]
    request_type: int
    hebbian_network: int
    context: str

    def __init__(
        self,
        list: _Optional[_Iterable[str]] = ...,
        request_type: _Optional[int] = ...,
        hebbian_network: _Optional[int] = ...,
        context: _Optional[str] = ...,
    ) -> None: ...

class HandleCount(_message.Message):
    __slots__ = ("map", "request_type", "hebbian_network", "context")

    class MapEntry(_message.Message):
        __slots__ = ("key", "value")
        KEY_FIELD_NUMBER: _ClassVar[int]
        VALUE_FIELD_NUMBER: _ClassVar[int]
        key: str
        value: int
        def __init__(self, key: _Optional[str] = ..., value: _Optional[int] = ...) -> None: ...

    MAP_FIELD_NUMBER: _ClassVar[int]
    REQUEST_TYPE_FIELD_NUMBER: _ClassVar[int]
    HEBBIAN_NETWORK_FIELD_NUMBER: _ClassVar[int]
    CONTEXT_FIELD_NUMBER: _ClassVar[int]

    map: _containers.ScalarMap[str, int]
    request_type: int
    hebbian_network: int
    context: str

    def __init__(
        self,
        map: _Optional[_Mapping[str, int]] = ...,
        request_type: _Optional[int] = ...,
        hebbian_network: _Optional[int] = ...,
        context: _Optional[str] = ...,
    ) -> None: ...

class ImportanceList(_message.Message):
    __slots__ = ("list",)
    LIST_FIELD_NUMBER: _ClassVar[int]
    list: _containers.RepeatedScalarFieldContainer[float]

    def __init__(self, list: _Optional[_Iterable[float]] = ...) -> None: ...
