from typing import ClassVar as _ClassVar
from typing import Iterable as _Iterable
from typing import Optional as _Optional

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf.internal import containers as _containers

DESCRIPTOR: _descriptor.FileDescriptor


class MessageData(_message.Message):
    __slots__ = ("command", "args", "sender", "is_broadcast", "visited_recipients")

    COMMAND_FIELD_NUMBER: _ClassVar[str]
    ARGS_FIELD_NUMBER: _ClassVar[_Iterable[str]]
    SENDER_FIELD_NUMBER: _ClassVar[str]
    IS_BROADCAST_FIELD_NUMBER: _ClassVar[bool]
    VISITED_RECIPIENTS_FIELD_NUMBER: _ClassVar[_Iterable[str]]

    command: str
    args: _containers.RepeatedScalarFieldContainer[str]
    sender: str
    is_broadcast: bool
    visited_recipients: _containers.RepeatedScalarFieldContainer[str]

    def __init__(
        self,
        command: str = ...,
        args: _Iterable[str] = ...,
        sender: str = ...,
        is_broadcast: _Optional[bool] = ...,
        visited_recipients: _Optional[_Iterable[str]] = ...,
    ) -> None: ...


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