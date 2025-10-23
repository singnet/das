import abc
import grpc
from concurrent import futures
import hyperon_das._grpc.atom_space_node_pb2_grpc as atom__space__node__pb2__grpc
import hyperon_das._grpc.atom_space_node_pb2 as atom__space__node__pb2
import hyperon_das._grpc.common_pb2 as common__pb2

from hyperon_das.service_bus.port_pool import PortPool
from hyperon_das.logger import log


class BaseCommandProxy(abc.ABC):
    """Base class for command proxies, handling configuration and shutdown of proxy nodes."""

    def __init__(self, command: str = None, args: list[str] = None) -> None:
        self.command = command
        self.args = args or []
        self.proxy_port: int = 0
        self.proxy_node: 'AtomSpaceNodeManager' = None
        self.requestor_id: str = None
        self.serial: int = None

    @abc.abstractmethod
    def process_message(self, msg: list[str]) -> None:
        """Processes received messages. Must be implemented by subclasses."""

    def setup_proxy_node(self, client_id: str = "", server_id: str = "") -> None:
        """Sets up the proxy node based on client and server IDs."""
        if self.proxy_port == 0:
            raise RuntimeError("Proxy node can't be set up")
        else:
            if client_id == "":
                id = self.requestor_id
                requestor_host = id.split(":")[0]
                requestor_id = requestor_host + ":" + str(self.proxy_port)
                self.proxy_node = AtomSpaceNodeManager(node_id=requestor_id, server_id=server_id, proxy=self)
            else:
                self.proxy_node = AtomSpaceNodeManager(node_id=client_id, server_id=server_id, proxy=self)
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


class AtomSpaceNodeManager:
    """Manages the AtomSpace node, including the gRPC server lifecycle."""
    def __init__(self, node_id: str, server_id: str, proxy: BaseCommandProxy) -> None:
        self.server = None
        self.peer_id = None
        self.node_id = node_id
        self.server_id = server_id
        self.servicer = AtomSpaceNodeServicer(proxy)

    def start(self):
        self.server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        atom__space__node__pb2__grpc.add_AtomSpaceNodeServicer_to_server(self.servicer, self.server)
        self.server.add_insecure_port(self.node_id)
        self.server.start()

    def stop(self, grace=None):
        if self.server:
            self.server.stop(grace)

    def wait_for_termination(self):
        if self.server:
            self.server.wait_for_termination()


class AtomSpaceNodeServicer(atom__space__node__pb2__grpc.AtomSpaceNodeServicer):
    """Implements the gRPC service for the AtomSpace node."""
    def __init__(self, proxy: BaseCommandProxy):
        self.proxy = proxy

    def ping(self, request=None, context=None):
        return common__pb2.Ack(error=False, msg="ack")

    def execute_message(self, request: atom__space__node__pb2.MessageData, context=None):
        log.info(f"Remote command: <{request.command}> arrived at AtomSpaceNodeServicer {self.proxy.proxy_node.node_id}")
        log.debug(f"Request command: {request.command}, args: {request.args}, sender: {request.sender}, is_broadcast: {request.is_broadcast}, visited_recipients: {request.visited_recipients}")
        if request.command in ["query_answer_tokens_flow", "bus_command_proxy"]:
            self.proxy.process_message(request.args)
        return common__pb2.Empty()
