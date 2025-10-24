import enum
import grpc

from hyperon_das._grpc.atom_space_node_pb2_grpc import AtomSpaceNodeStub
from hyperon_das._grpc import atom_space_node_pb2

from hyperon_das.logger import log


class BusCommand(str, enum.Enum):
    PATTERN_MATCHING_QUERY = "pattern_matching_query"


class Bus:
    def __init__(self, other: 'Bus' = None) -> None:
        if other is not None:
            self._command_owner = dict(other._command_owner)
        else:
            self._command_owner = {}

    def add(self, command: str) -> None:
        if command in self._command_owner:
            if self._command_owner[command] != "":
                raise RuntimeError(f"Bus: command <{command}> is already assigned to {self._command_owner[command]}")
        else:
            self._command_owner[command] = ""

    def set_ownership(self, command: str, node_id: str) -> None:
        if command not in self._command_owner:
            raise RuntimeError(f"Bus: command <{command}> is not defined")
        else:
            if self._command_owner[command] == "":
                self._command_owner[command] = node_id
            else:
                if self._command_owner[command] != node_id:
                    raise RuntimeError(f"Bus: command <{command}> is already assigned to {self._command_owner[command]}")

    def get_ownership(self, command: str) -> str:
        if command not in self._command_owner:
            raise RuntimeError(f"Bus: unknown command <{command}>")
        return self._command_owner[command]

    def contains(self, command: str) -> bool:
        return command in self._command_owner

    def to_string(self) -> str:
        entries = []
        for cmd, owner in self._command_owner.items():
            if owner:
                entries.append(f"{cmd}:{owner}")
            else:
                entries.append(cmd)
        return "{" + ", ".join(entries) + "}"

    def __eq__(self, other: 'Bus') -> bool:
        return self._command_owner == other._command_owner

    def __add__(self, command: str) -> 'Bus':
        self.add(command)
        return self


class BusNode:
    def __init__(self, node_id: str, node_commands: list[str], known_peer: str = ""):
        self._bus = Bus()
        self.node_id = node_id
        for command in node_commands:
            self._bus.add(command)
            self._bus.set_ownership(command, known_peer)

    def send_bus_command(self, command: str, args: list[str]) -> None:
        target_id = self._bus.get_ownership(command)
        if target_id == "":
            raise RuntimeError(f"Bus: no owner is defined for command <{command}>")
        else:
            log.info(f"BUS node {self.node_id} is routing command <{command}> to {target_id}")
            self.send(command, args, target_id)

    def send(self, command: str, args: list[str], target_id: str) -> None:
        with grpc.insecure_channel(target_id) as channel:
            stub = AtomSpaceNodeStub(channel)
            log.debug(f"Sending command: {command} with args: {args} to target: {target_id}")
            message = atom_space_node_pb2.MessageData(
                command=command,
                args=args,
                sender=self.node_id,
                is_broadcast=False,
                visited_recipients=[]
            )
            try:
                stub.execute_message(message)
            except grpc.RpcError as e:
                log.error(f"Failed to send message: {e}")
