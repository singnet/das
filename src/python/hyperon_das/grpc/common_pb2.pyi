from typing import ClassVar as _ClassVar
from typing import Iterable as _Iterable
from typing import Mapping as _Mapping
from typing import Optional as _Optional

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf.internal import containers as _containers

DESCRIPTOR: _descriptor.FileDescriptor

class Empty(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class Ack(_message.Message):
    __slots__ = ("error", "msg")
    ERROR_FIELD_NUMBER: _ClassVar[int]
    MSG_FIELD_NUMBER: _ClassVar[int]
    error: bool
    msg: str
    def __init__(self, error: bool = ..., msg: _Optional[str] = ...) -> None: ...

class HandleList(_message.Message):
    __slots__ = ("handle_list",)
    HANDLE_LIST_FIELD_NUMBER: _ClassVar[int]
    handle_list: _containers.RepeatedScalarFieldContainer[str]
    def __init__(self, handle_list: _Optional[_Iterable[str]] = ...) -> None: ...

class HandleCount(_message.Message):
    __slots__ = ("handle_count",)

    class HandleCountEntry(_message.Message):
        __slots__ = ("key", "value")
        KEY_FIELD_NUMBER: _ClassVar[int]
        VALUE_FIELD_NUMBER: _ClassVar[int]
        key: str
        value: int
        def __init__(self, key: _Optional[str] = ..., value: _Optional[int] = ...) -> None: ...

    HANDLE_COUNT_FIELD_NUMBER: _ClassVar[int]
    handle_count: _containers.ScalarMap[str, int]
    def __init__(self, handle_count: _Optional[_Mapping[str, int]] = ...) -> None: ...
