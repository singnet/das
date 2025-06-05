import abc
import grpc
import threading
from concurrent import futures
from queue import Queue, Empty
import python_bus_client._grpc.atom_space_node_pb2_grpc as atom__space__node__pb2__grpc
import python_bus_client._grpc.atom_space_node_pb2 as atom__space__node__pb2
import python_bus_client._grpc.common_pb2 as common__pb2

from python_bus_client.bus import BusCommand
from python_bus_client.port_pool import PortPool
from python_bus_client.logger import log


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
                self.proxy_node = AtomSpaceNodeManager(node_id=requestor_id, server_id=server_id, handler=self)
            else:
                self.proxy_node = AtomSpaceNodeManager(node_id=client_id, server_id=server_id, handler=self)
                self.proxy_node.peer_id = server_id
            self.proxy_node.start()

    def graceful_shutdown(self):
        """Performs a graceful shutdown of the proxy node, releasing the port."""
        log.info(f"graceful_shutdown port: {self.proxy_port}")
        if self.proxy_port != 0:
            if self.proxy_node:
                self.proxy_node.stop()
                self.proxy_node.wait_for_termination()
            PortPool.return_port(self.proxy_port)


class PatternMatchingQueryHandler(BaseCommandProxy):
    """Handler for pattern matching queries, managing execution and results."""

    ABORT = "abort"
    ANSWER_BUNDLE = "answer_bundle"
    COUNT = "count"
    FINISHED = "finished"

    def __init__(
        self,
        tokens: list[str],
        context: str = "",
        unique_assignment: bool = False,
        update_attention_broker: bool = False,
        count_only: bool = False
    ) -> None:
        super().__init__(command=BusCommand.PATTERN_MATCHING_QUERY, args=[
            context,
            str(unique_assignment),
            str(update_attention_broker),
            str(count_only),
        ] + tokens)
        self.answer_queue = Queue()
        self._lock = threading.Lock()
        self.answer_flow_finished = False
        self.abort_flag = False
        self.update_attention_broker = update_attention_broker
        self.answer_count = 0
        self.count_flag = count_only
        self.unique_assignment_flag = unique_assignment

    def finished(self) -> bool:
        """Checks if the answers is finished or aborted."""
        with self._lock:
            return (
                self.abort_flag or (
                    self.answer_flow_finished and (self.count_flag or self.answer_queue.empty())
                )
            )

    def pop(self):
        """Pops and returns the next item from the answer queue, if available."""
        with self._lock:
            if self.count_flag:
                log.warning("Can't pop QueryAnswers from count_only queries.")
                return None
            if self.abort_flag:
                log.warning(f"abort_flag: {self.abort_flag}")
                return None
            try:
                return self.answer_queue.get(block=False)
            except Empty:
                return None

    def process_message(self, msg: list[str]) -> None:
        """Processes received messages, updating the answer queue or flags."""
        with self._lock:
            for item in msg:
                if item == self.FINISHED:
                    if not self.abort_flag:
                        self.answer_flow_finished = True
                    break
                elif item == self.ABORT:
                    self.abort_flag = True
                    break
                elif item in [self.ANSWER_BUNDLE, self.COUNT]:
                    continue
                else:
                    self.answer_count += 1
                    self.answer_queue.put(item)


class AtomSpaceNodeManager:
    """Manages the AtomSpace node, including the gRPC server lifecycle."""
    def __init__(self, node_id: str, server_id: str, handler: BaseCommandProxy) -> None:
        self.server = None
        self.peer_id = None
        self.node_id = node_id
        self.server_id = server_id
        self.servicer = AtomSpaceNodeServicer(handler)

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
    def __init__(self, handler: BaseCommandProxy):
        self.handler = handler

    def ping(self, request=None, context=None):
        return common__pb2.Ack(error=False, msg="ack")

    def execute_message(self, request: atom__space__node__pb2.MessageData, context=None):
        log.info(f"Remote command: <{request.command}> arrived at AtomSpaceNodeServicer {self.handler.proxy_node.node_id}")
        if request.command in ["query_answer_tokens_flow", "bus_command_proxy"]:
            self.handler.process_message(request.args)
        return common__pb2.Empty()
