import abc
from concurrent import futures

import grpc

import hyperon_das._grpc.common_pb2 as common__pb2
import hyperon_das._grpc.distributed_algorithm_node_pb2 as distributed__algorithm__node__pb2
import hyperon_das._grpc.distributed_algorithm_node_pb2_grpc as distributed__algorithm__node__pb2__grpc
from hyperon_das._grpc.distributed_algorithm_node_pb2_grpc import DistributedAlgorithmNodeStub
from hyperon_das.logger import log
from hyperon_das.service_bus.port_pool import PortPool


class BusCommandProxy(abc.ABC):
    """Base class for command proxies, handling configuration and shutdown of proxy nodes."""

    def __init__(self, command: str = None, args: list[str] = None) -> None:
        self.command = command
        self.args = args or []
        self.proxy_port: int = 0
        self.proxy_node: 'DistributedAlgorithmNodeManager' = None
        self.requestor_id: str = None
        self.serial: int = None

    @abc.abstractmethod
    def process_message(self, msg: list[str]) -> bool:
        """Processes received messages. Must be implemented by subclasses."""

    @abc.abstractmethod
    def pack_command_line_args(self) -> None: ...

    def setup_proxy_node(self, client_id: str = "", server_id: str = "") -> None:
        """Sets up the proxy node based on client and server IDs."""
        if self.proxy_port == 0:
            raise RuntimeError("Proxy node can't be set up")
        else:
            if client_id == "":
                id = self.requestor_id
                requestor_host = id.split(":")[0]
                requestor_id = requestor_host + ":" + str(self.proxy_port)
                self.proxy_node = DistributedAlgorithmNodeManager(node_id=requestor_id, server_id=server_id, proxy=self)
            else:
                self.proxy_node = DistributedAlgorithmNodeManager(node_id=client_id, server_id=server_id, proxy=self)
                self.proxy_node.peer_id = server_id
            self.proxy_node.start()

    def graceful_shutdown(self):
        """Performs a graceful shutdown of the proxy node, releasing the port."""
        if self.proxy_port != 0:
            log.info(f"graceful_shutdown port: {self.proxy_port}")
            if self.proxy_node:
                self.proxy_node.stop()
                self.proxy_node.wait_for_termination()
            PortPool.return_port(self.proxy_port)

    def to_remote_peer(self, command: str, args: list[str]) -> None:
        log.debug(f"{self.proxy_node.node_id} is issuing proxy command <{command}>")
        self.proxy_node.to_remote_peer(command, args)


class DistributedAlgorithmNodeManager:
    """Manages the AtomSpace node, including the gRPC server lifecycle."""

    PROXY_COMMAND = "bus_command_proxy"

    def __init__(self, node_id: str, server_id: str, proxy: BusCommandProxy) -> None:
        self.server = None
        self.peer_id = None
        self.node_id = node_id
        self.server_id = server_id
        self.servicer = DistributedAlgorithmNodeServicer(proxy)

    def start(self):
        self.server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        distributed__algorithm__node__pb2__grpc.add_DistributedAlgorithmNodeServicer_to_server(
            self.servicer, self.server
        )
        self.server.add_insecure_port(self.node_id)
        self.server.start()

    def stop(self, grace=None):
        if self.server:
            self.server.stop(grace)

    def wait_for_termination(self):
        if self.server:
            self.server.wait_for_termination()

    def node_joined_network(self, node_id: str) -> None:
        self.peer_id = node_id

    def to_remote_peer(self, command: str, args: list[str]) -> None:
        if not self.peer_id:
            msg = "Unknown peer"
            log.error(msg)
            raise RuntimeError(msg)

        new_args = args
        new_args.append(command)

        with grpc.insecure_channel(self.peer_id) as channel:
            stub = DistributedAlgorithmNodeStub(channel)
            log.debug(f"Sending command: {self.PROXY_COMMAND} to target: {self.peer_id}")
            message = distributed__algorithm__node__pb2.MessageData(
                command=self.PROXY_COMMAND,
                args=new_args,
                sender=self.node_id,
                is_broadcast=False,
                visited_recipients=[],
            )
            try:
                stub.execute_message(message)
            except grpc.RpcError as e:
                log.error(f"Failed to send message: {e}")


class DistributedAlgorithmNodeServicer(distributed__algorithm__node__pb2__grpc.DistributedAlgorithmNodeServicer):
    """Implements the gRPC service for the AtomSpace node."""

    def __init__(self, proxy: BusCommandProxy):
        self.proxy = proxy

    def ping(self, request=None, context=None):
        return common__pb2.Ack(error=False, msg="ack")

    def execute_message(self, request: distributed__algorithm__node__pb2.MessageData, context=None):
        log.info(
            f"Remote command: <{request.command}> arrived at DistributedAlgorithmNodeServicer {self.proxy.proxy_node.node_id}"
        )
        log.debug(
            f"Request command: {request.command}, args: {request.args}, sender: {request.sender}, is_broadcast: {request.is_broadcast}, visited_recipients: {request.visited_recipients}"
        )
        if request.command == "node_joined_network":
            self.proxy.proxy_node.node_joined_network(request.args[0])
        if request.command in ["query_answer_tokens_flow", "bus_command_proxy"]:
            self.proxy.process_message(request.args)
        return common__pb2.Empty()
