import grpc
from concurrent import futures

import atom_space_node_grpc.atom_space_node_pb2_grpc as atom__space__node__pb2__grpc
import atom_space_node_grpc.atom_space_node_pb2 as atom__space__node__pb2
import atom_space_node_grpc.common_pb2 as common__pb2


# WIP

class PatternMatchingQueryProxy:
    ABORT = "ABORT"           # Abort current query
    ANSWER_BUNDLE = "ANSWER_BUNDLE"  # Delivery of a bundle with QueryAnswer objects
    COUNT = "COUNT"           # Delivery of the final result of a count_only query
    FINISHED = "FINISHED"     # Notification that all query results have already been delivered

    def __init__(
        self,
        tokens: list[str] = None,
        context: str = "",
        unique_assignment: bool = False,
        update_attention_broker: bool = False,
        count_only: bool = False
    ) -> None:
        pass

    def pop(self) -> None:
        pass
    
    def finished(self) -> None:
        pass
    
    def get_count(self) -> None:
        pass


class ProxyNode:
    pass


class StarNode(atom__space__node__pb2__grpc.AtomSpaceNodeServicer):
    def __init__(self, node_id: str, proxy: PatternMatchingQueryProxy):
        self.node_id = node_id
        self.proxy = proxy

    def process_message(self, msg: atom__space__node__pb2.MessageData):
        print(f"StarNode.process_message()[{self.node_id}]: MessageData -> len={len(msg.args)}")

        if msg.command in ["query_answer_tokens_flow", "bus_command_proxy"]:
            args = msg.args
        else:
            args = []

        with self.proxy.lock:
            for arg in args:
                if arg == PatternMatchingQueryProxy.FINISHED:
                    self.proxy.answer_flow_finished = True
                    break
                elif arg == PatternMatchingQueryProxy.ABORT:
                    self.proxy.abort_flag = True
                    break
                elif arg in [PatternMatchingQueryProxy.ANSWER_BUNDLE, PatternMatchingQueryProxy.COUNT]:
                    continue
                else:
                    self.proxy.answer_count += 1
                    self.proxy.answer_queue.append(arg)

    def execute_message(self, request: atom__space__node__pb2.MessageData, context):
        self.process_message(request)
        return common__pb2.Empty()

    def ping(self, request, context):
        return common__pb2.Ack(error=False, msg="ack")

    def start_server(self):
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        atom__space__node__pb2__grpc.add_AtomSpaceNodeServicer_to_server(self, server)
        server.add_insecure_port(self.node_id)
        server.start()
        server.wait_for_termination()
