from hyperon_das_node.hyperon_das_node_ext import (
    LeadershipBrokerType,
    MessageBrokerType,
    DistributedAlgorithmNode,
    Message,
    StarNode,
)
from typing import List
from queue import Queue
from threading import Thread


class DASLinkCreateMessage(Message):
    def __init__(self, command: str, args: List[str]):
        super().__init__()
        self.message = args

    def act(self, node, *args, **kwargs):
        node.add_request(self.message)


class DASAgentNode(StarNode):
    def __init__(self, node_id: str = None, server_id: str = None):
        if server_id is None:
            super().__init__(node_id)
        else:
            super().__init__(node_id, server_id)
        self.requests = Queue()

    def message_factory(self, command: str, args: list) -> Message:
        message = super().message_factory(command, args)
        if message:
            return message
        if command == "create_link":
            return DASLinkCreateMessage(command, args)
        return None

    def send_message(self, command: str, args: list, receiver_id: str):
        self.send(command, args, receiver_id)

    def add_request(self, request):
        self.requests.put(request)

    def pop_request(self):
        return self.requests.get()

    def has_request(self):
        return not self.requests.empty()
