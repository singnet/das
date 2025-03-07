import threading
from abc import ABC, abstractmethod


class QueryElement(ABC):
    def __init__(self):
        self.id = ""
        self.subsequent_id = ""
        self.flow_finished = False
        self.is_terminal = False
        self.flow_finished_lock = threading.Lock()

    @abstractmethod
    def setup_buffers(self):
        """
        Abstract method that must be implemented in a subclass
        to setup buffers.
        """
        pass

    @abstractmethod
    def graceful_shutdown(self):
        """
        Abstract method that must be implemented in a subclass
        for graceful shutdown behavior.
        """
        pass

    def is_flow_finished(self) -> bool:
        """
        Check if the flow has finished with thread safety.
        """
        with self.flow_finished_lock:
            return self.flow_finished

    def set_flow_finished(self):
        """
        Set the flow as finished with thread safety.
        """
        with self.flow_finished_lock:
            self.flow_finished = True
