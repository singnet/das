import threading
import grpc
from concurrent import futures

from atom_space_node_grpc import atom_space_node_pb2_grpc
from atom_space_node_grpc import atom_space_node_pb2


class PatternMatchingQueryProxy:
    def __init__(self, tokens: list[str], context: str, unique_assignment: bool, update_attention_broker: bool, count_only: bool):
        pass

    def pop(self) -> None:
        pass
    
    def finished(self) -> None:
        pass
    
    def get_count(self) -> None:
        pass


class ProxyNode:
    pass


# WIP
class StarNode(atom_space_node_pb2_grpc.AtomSpaceNodeServicer):
    def __init__(self, node_id: str, proxy: PatternMatchingQueryProxy):
        self.node_id = node_id
        self.proxy = proxy

    def process_message(self, msg: atom_space_node_pb2.MessageData):
        print(f"StarNode::process_message()[{self.node_id}]: MessageData -> len={len(msg.args)}")

        if msg.command in ["query_answer_tokens_flow", "bus_command_proxy"]:
            args = msg.args
        else:
            args = []

        with self.proxy.lock:
            for arg in args:
                if arg == "FINISHED":
                    self.proxy.answer_flow_finished = True
                    break
                elif arg == "ABORT":
                    self.proxy.abort_flag = True
                    break
                elif arg in ["ANSWER_BUNDLE", "COUNT"]:
                    continue
                else:
                    self.proxy.answer_count += 1
                    self.proxy.answer_queue.append(arg)

    def execute_message(self, request: atom_space_node_pb2.MessageData, context):
        self.process_message(request)
        return atom_space_node_pb2.Empty()

    def ping(self, request, context):
        return atom_space_node_pb2.Ack(error=False, msg="ack")

    def start_server(self):
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        atom_space_node_pb2_grpc.add_AtomSpaceNodeServicer_to_server(self, server)
        server.add_insecure_port(self.node_id)
        server.start()
        server.wait_for_termination()
