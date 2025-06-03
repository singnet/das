from queue import Queue, Empty
from src.logger import log


class PortPool:
    _pool: Queue = None
    _port_lower: int = 0
    _port_upper: int = 0

    @classmethod
    def initialize_statics(cls, port_lower: int, port_upper: int) -> None:
        if port_lower > port_upper:
            raise ValueError(f"Invalid port limits [{port_lower}..{port_upper}]")
        log.info(f"Port range: [{port_lower} : {port_upper}]")
        cls._port_lower = port_lower
        cls._port_upper = port_upper
        cls._pool = Queue(maxsize=port_upper - port_lower + 1)
        for port in range(port_lower, port_upper + 1):
            cls._pool.put(port)

    @classmethod
    def get_port(cls) -> int:
        if cls._pool is None:
            raise ValueError("PortPool not initialized. Call initialize_statics first.")

        try:
            port = cls._pool.get(timeout=0.1)
        except Empty:
            raise ValueError(
                f"Unable to get available PORT number in [{cls._port_lower}..{cls._port_upper}]"
            )
        return port

    @classmethod
    def return_port(cls, port: int) -> None:
        if cls._pool is None:
            raise ValueError("PortPool not initialized. Call initialize_statics first.")

        if not (cls._port_lower <= port <= cls._port_upper):
            raise ValueError(f"Port out of the available range available [{cls._port_lower}..{cls._port_upper}]")

        cls._pool.put(port)
