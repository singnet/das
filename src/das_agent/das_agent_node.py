from hyperon_das_node import DistributedAlgorithmNode, Message
from hyperon_das_node.hyperon_das_node_ext import LeadershipBrokerType, MessageBrokerType
from typing import List
from queue import Queue

class DASLinkCreateMessage(Message):
    def __init__(self, command: str, args: List[str]):
        super().__init__()
        self.message = args

    def act(self, node, *args, **kwargs):
        node.add_request(self.message)

class StarNode(DistributedAlgorithmNode):
    is_server: bool
    server_id: str

    def __init__(
        self,
        node_id: str,
        server_id: str = None,
        messaging_backend: MessageBrokerType = MessageBrokerType.GRPC,
    ):
        # Call the parent constructor (DistributedAlgorithmNode)
        super().__init__(node_id, LeadershipBrokerType.SINGLE_MASTER_SERVER, messaging_backend)
        if server_id:
            # If server_id is provided, this is a client node
            self.server_id = server_id
            self.is_server = False
            self.add_peer(server_id)
        else:
            # If no server_id, this is a server node
            self.is_server = True

        # Join the network regardless of server/client
        self.join_network()

    def node_joined_network(self, node_id: str):
        if self.is_server:
            self.add_peer(node_id)

    def cast_leadership_vote(self) -> str:
        if self.is_server:
            return self.node_id()
        else:
            return self.server_id



class DASAgentNode(StarNode):
    def __init__(self, node_id: str = None, server_id: str = None):
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