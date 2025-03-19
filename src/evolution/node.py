from queue import Queue

from hyperon_das_node import Message

from evolution.das_node.star_node import StarNode


class EvolutionRequest(Message):
    def __init__(self, senders_id: str, request: str):
        super().__init__()
        self.senders = senders_id.split(',')
        self.request = request

    def act(self, node: "EvolutionNode") -> None:
        node.add_request(self.senders, self.request)


class EvolutionNode(StarNode):
    client_ip: str
    first_evolution_client_port: int
    last_evolution_client_port: int
    next_evolution_client_port: int

    def __init__(self, node_id: str = None, server_id: str = None) -> None:
        self.request_queue = Queue()
        self.known_commands = {
            "evolution_request": EvolutionRequest,
        }
        self.first_evolution_client_port = 70000
        self.last_evolution_client_port = 71999
        self.next_evolution_client_port = (self.first_evolution_client_port + self.last_evolution_client_port) // 2

        if not node_id:
            self.is_server = False
            self.client_ip = "localhost"
            node_id = self.next_id()

        super().__init__(node_id, server_id)

        self.client_ip = self.node_id().split(":")[0]

    def message_factory(self, command: str, args: list[str]) -> Message:
        """
        Creates a message object based on a command and arguments.

        Args:
            command (str): The command to execute.
            args (list[str]): Arguments to pass to the command.

        Returns:
            Message: The constructed message object.
        """
        message = super().message_factory(command, args)
        if message is not None:
            return message
        if message_class := self.known_commands.get(command):
            return message_class(*args)

    def add_request(self, senders: list[str], request: str) -> None:
        if self.is_server:
            self.request_queue.put({'senders': senders, 'data': request})

    def pop_request(self) -> dict:
        if self.is_server and not self.request_queue.empty():
            return self.request_queue.get()

    def next_id(self) -> str:
        if self.is_server:
            raise ValueError("'next_id()' is not available in EvolutionNode server")

        port = self.next_evolution_client_port
        limit = self.last_evolution_client_port

        if self.next_evolution_client_port > limit:
            self.next_evolution_client_port = (self.first_evolution_client_port + self.last_evolution_client_port) // 2

        self.next_evolution_client_port += 1
        return f"{self.ip}:{port}"
