from hyperon_das_node import DistributedAlgorithmNode, LeadershipBrokerType, MessageBrokerType
import time

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
