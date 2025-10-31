import time
import threading
from hyperon_das.service_bus.port_pool import PortPool
from hyperon_das.distributed_algorithm_node.bus_node import BusCommand, BusNode
from hyperon_das.service_bus.proxy import BusCommandProxy
from hyperon_das.logger import log


class ServiceBus:
    def __init__(self, host_id: str, known_peer: str, commands: list[str], port_lower: int, port_upper: int):
        self.service_list: list[str] = []
        self.next_request_serial: int = 1
        self.bus_node = BusNode(host_id, commands, known_peer)
        PortPool.initialize_statics(port_lower, port_upper)

    def issue_bus_command(self, proxy: 'BusCommandProxy') -> None:
        log.info(f"{self.bus_node.node_id} is issuing BUS command <{proxy.command}>")
        proxy.requestor_id = self.bus_node.node_id
        proxy.serial = self.next_request_serial
        self.next_request_serial += 1
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

            proxy.pack_command_line_args()

            for arg in proxy.args:
                args.append(arg)

            try:
                self.bus_node.send_bus_command(proxy.command, args)
                time.sleep(1)
            except Exception:
                proxy.abort_flag = False
                log.error(f"Failed to issue BUS command <{proxy.command}> with args {args}")
                raise


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
            [
                BusCommand.PATTERN_MATCHING_QUERY,
                BusCommand.ATOMDB
            ],
            port_lower,
            port_upper
        )

    def get_instance(self) -> 'ServiceBus':
        return self.service_bus
