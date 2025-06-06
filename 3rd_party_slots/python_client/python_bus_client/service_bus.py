import threading
from python_bus_client.port_pool import PortPool
from python_bus_client.bus_node import BusNode
from python_bus_client.proxy import PatternMatchingQueryHandler
from python_bus_client.bus import BusCommand
from python_bus_client.logger import log


class ServiceBus:
    def __init__(self, host_id: str, known_peer: str, commands: list[str], port_lower: int, port_upper: int):
        self.service_list: list[str] = []
        self.next_request_serial: int = 0
        self.bus_node: 'BusNode' = BusNode(host_id, commands, known_peer)
        PortPool.initialize_statics(port_lower, port_upper)

    def issue_bus_command(self, proxy: 'PatternMatchingQueryHandler'):
        log.info(f"{self.bus_node.node_id} is issuing BUS command <{proxy.command}>")

        self.next_request_serial += 1
        proxy.requestor_id = self.bus_node.node_id
        proxy.serial = self.next_request_serial
        proxy.proxy_port = PortPool.get_port()

        if proxy.proxy_port == 0:
            raise RuntimeError("No port is available to start bus command proxy")
        else:
            proxy.setup_proxy_node()

            args = [
                proxy.requestor_id,
                str(proxy.serial),
                proxy.proxy_node.node_id
            ]
            args.extend(proxy.args)

            # TODO
            try:
                self.bus_node.send_bus_command(proxy.command, args)
            except Exception:
                pass


class ServiceBusSingletonMeta(type):
    """
    This is a thread-safe implementation of Singleton.
    """

    _instances = {}

    _lock = threading.Lock()

    def __call__(cls, *args, **kwargs):
        with cls._lock:
            if cls not in cls._instances:
                instance = super().__call__(*args, **kwargs)
                cls._instances[cls] = instance
        return cls._instances[cls]


class ServiceBusSingleton(metaclass=ServiceBusSingletonMeta):
    def __init__(self, host_id: str, known_peer: str, port_lower: int, port_upper: int) -> None:
        self.service_bus = ServiceBus(
            host_id,
            known_peer,
            [BusCommand.PATTERN_MATCHING_QUERY],
            port_lower,
            port_upper
        )

    def get_instance(self) -> 'ServiceBus':
        return self.service_bus
