import grpc

from src.grpc.atom_space_node_pb2_grpc import AtomSpaceNodeStub
from src.grpc import atom_space_node_pb2

from src.bus import Bus
from src.logger import log


class BusNode:
    def __init__(self, node_id: str, node_commands: set[str], known_peer: str = ""):
        self._node_id = node_id
        self._bus = Bus()
        for command in node_commands:
            self._bus.add(command)
            self._bus.set_ownership(command, known_peer)

    def send_bus_command(self, command: str, args: list[str]) -> None:
        target_id = self._bus.get_ownership(command)
        if target_id == "":
            raise RuntimeError(f"Bus: no owner is defined for command <{command}>")
        else:
            log.info(f"BUS node {self.node_id()} is routing command {command} to {target_id}")
            self.send(command, args, target_id)

    def node_id(self):
        return self._node_id

    def send(self, command: str, args: list[str], target_id: str) -> None:
        with grpc.insecure_channel(target_id) as channel:
            stub = AtomSpaceNodeStub(channel)
            message = atom_space_node_pb2.MessageData(
                command=command,
                args=args,
                sender=self.node_id(),
                is_broadcast=False,
                visited_recipients=[]
            )
            try:
                stub.execute_message(message)
            except grpc.RpcError as e:
                log.error(f"Error: {e}")
