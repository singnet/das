import itertools

from queue import Queue

from hyperon_das_node import Message

from evolution.das_node.star_node import StarNode


class NodeIdFactory:
    def __init__(self, ip: str = "localhost", start_port: int = 70000, end_port: int = 71999):
        self.ip = ip
        self.port_generator = self._port_sequence(start_port, end_port)

    def _port_sequence(self, start: int, end: int):
        """Generates a cyclic sequence of gates between start and end."""
        return itertools.cycle(range(start, end + 1))

    def generate_node_id(self) -> str:
        """Generates a new node_id in the format 'ip:port'."""
        port = next(self.port_generator)
        return f"{self.ip}:{port}"


class EvolutionRequest(Message):
    def __init__(self, senders_id: str, request: str, *args, **kwargs):
        super().__init__()
        self.senders = senders_id.split(',')
        self.request = request

    def act(self, node: "EvolutionNode") -> None:
        node.add_request(self.senders, self.request)


class EvolutionNode(StarNode):
    def __init__(self, node_id: str = None, server_id: str = None, node_id_factory: NodeIdFactory = None) -> None:
        self.request_queue = Queue()
        self.known_commands = {
            "evolution_request": EvolutionRequest,
        }
        if not node_id:
            if node_id_factory is None:
                raise ValueError("For clients, a node_id factory must be provided")
            node_id = node_id_factory.generate_node_id()
        super().__init__(node_id, server_id)

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
