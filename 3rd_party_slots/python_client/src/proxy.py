import grpc
import threading
from concurrent import futures
from queue import Queue, Empty
import src.grpc.atom_space_node_pb2_grpc as atom__space__node__pb2__grpc
import src.grpc.atom_space_node_pb2 as atom__space__node__pb2
import src.grpc.common_pb2 as common__pb2

from src.bus import BusCommand
from src.port_pool import PortPool
from src.logger import log


class BusCommandProxy:
    def __init__(self, command: str = None, args: list[str] = None):
        self.proxy_port: int = 0
        self.command: str = command
        self.args: list[str] = args
        self.proxy_node: 'ProxyNode' = None
        self.requestor_id: str = None
        self.serial: int = None

    def setup_proxy_node(self, client_id: str = "", server_id: str = ""):
        if self.proxy_port == 0:
            raise RuntimeError("Proxy node can't be set up")
        else:
            if client_id == "":
                id = self.requestor_id
                requestor_host = id.split(":")[0]
                requestor_id = requestor_host + ":" + str(self.proxy_port)
                self.proxy_node = ProxyNode(proxy=self, node_id=requestor_id, server_id=server_id)
            else:
                self.proxy_node = ProxyNode(proxy=self, node_id=client_id, server_id=server_id)
                self.proxy_node.peer_id = server_id

    def graceful_shutdown(self):
        log.info(f"graceful_shutdown port: {self.proxy_port}")
        if self.proxy_port != 0:
            PortPool.return_port(self.proxy_port)
            self.proxy_node.graceful_shutdown()


class PatternMatchingQueryProxy(BusCommandProxy):
    ABORT = "abort"           # Abort current query
    ANSWER_BUNDLE = "answer_bundle"  # Delivery of a bundle with QueryAnswer objects
    COUNT = "count"           # Delivery of the final result of a count_only query
    FINISHED = "finished"     # Notification that all query results have already been delivered

    def __init__(
        self,
        tokens: list[str] = None,
        context: str = "",
        unique_assignment: bool = False,
        update_attention_broker: bool = False,
        count_only: bool = False
    ) -> None:
        super().__init__()
        self.answer_queue = Queue()
        self.api_mutex = threading.Lock()
        
        self.answer_flow_finished = False
        self.abort_flag = False
        self.update_attention_broker = False
        self.answer_count = 0

        self.command = BusCommand.PATTERN_MATCHING_QUERY
        self.count_flag = count_only
        self.unique_assignment_flag = unique_assignment        
        self.args = [
            context,
            str(unique_assignment),
            str(update_attention_broker),
            str(count_only),
        ] + tokens

    def finished(self) -> bool:
        with self.api_mutex:
            return (
                self.abort_flag or (
                    self.answer_flow_finished and (self.count_flag or self.answer_queue.empty())
                )
            )

    def pop(self):
        with self.api_mutex:
            if self.count_flag:
                print("Can't pop QueryAnswers from count_only queries.")
                return None
            if self.abort_flag:
                print(f"abort_flag: {self.abort_flag}")
                return None
            try:
                return self.answer_queue.get(block=False)
            except Empty:
                return None
        print("proxy.pop END")

    def get_count(self) -> None:
        pass


class ProxyNode:
    def __init__(self, proxy: 'PatternMatchingQueryProxy', node_id: str, server_id: str) -> None:
        self.peer_id = None
        self._node_id = node_id
        self._server_id = server_id
        self.service = NodeServicer(node_id, proxy)
        self.service.start()

    def node_id(self):
        return self._node_id

    def server_id(self):
        return self._server_id

    def graceful_shutdown(self):
        self.service.stop()


class NodeServicer(atom__space__node__pb2__grpc.AtomSpaceNodeServicer):
    def __init__(self, node_id: str, proxy: PatternMatchingQueryProxy):
        self.node_id = node_id
        self.proxy = proxy
        self.server = None

    def ping(self, request=None, context=None):
        return common__pb2.Ack(error=False, msg="ack")

    def execute_message(self, request: atom__space__node__pb2.MessageData, context=None):
        self.process_message(request)
        return common__pb2.Empty()

    def process_message(self, msg: atom__space__node__pb2.MessageData):      
        log.info(f"Remote command: <{msg.command}> arrived at NodeServicer {self.node_id}")

        if msg.command in ["query_answer_tokens_flow", "bus_command_proxy"]:
            args = msg.args
        else:
            args = []

        with self.proxy.api_mutex:
            for arg in args:
                if arg == PatternMatchingQueryProxy.FINISHED:
                    if not self.proxy.abort_flag:
                        self.proxy.answer_flow_finished = True
                    break
                elif arg == PatternMatchingQueryProxy.ABORT:
                    self.proxy.abort_flag = True
                    break
                elif arg in [PatternMatchingQueryProxy.ANSWER_BUNDLE, PatternMatchingQueryProxy.COUNT]:
                    continue
                else:
                    self.proxy.answer_count += 1
                    self.proxy.answer_queue.put(arg)

    def start(self):
        self.server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        atom__space__node__pb2__grpc.add_AtomSpaceNodeServicer_to_server(self, self.server)
        self.server.add_insecure_port(self.node_id)
        self.server.start()

    def stop(self, grace=None):
        if self.server:
            self.server.stop(grace)

    def wait_for_termination(self):
        if self.server:
            self.server.wait_for_termination()
        
    # async approach
    
    # async def start(self):
    #     self.server = grpc.aio.server(futures.ThreadPoolExecutor(max_workers=10))
    #     atom__space__node__pb2__grpc.add_AtomSpaceNodeServicer_to_server(self, self.server)
    #     self.server.add_insecure_port(self.node_id)
    #     await self.server.start()
    
    # async def stop(self, grace=None):
    #     if self.server:
    #         await self.server.stop(grace)
    
    # async def wait_for_termination(self):
    #     if self.server:
    #         await self.server.wait_for_termination()
