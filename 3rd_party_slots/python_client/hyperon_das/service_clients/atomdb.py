import threading
from queue import Queue, Empty

from hyperon_das.distributed_algorithm_node.bus_node import BusCommand
from hyperon_das.service_bus.proxy import BaseCommandProxy
from hyperon_das.logger import log
from hyperon_das.commons.properties import Properties


class AtomDBProxy(BaseCommandProxy):

    ADD_ATOMS = "add_atoms"

    def __init__(self):
        super().__init__(command=BusCommand.ATOMDB)