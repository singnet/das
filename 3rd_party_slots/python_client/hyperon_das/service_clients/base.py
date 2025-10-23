import threading
from hyperon_das.commons.properties import Properties


class BaseProxy:
    ABORT = "abort"
    FINISHED = "finished"
    
    def __init__(self):
        self._lock = threading.Lock()
        self.abort_flag = False
        self.command_finished_flag = False
        self.parameter = Properties()
    
    def finished(self) -> bool:
        """Checks if the answers is finished or aborted."""
        with self._lock:
            return self.abort_flag or self.command_finished_flag
        
    def abort(self) -> None:
        with self._lock:
            if not self.command_finished_flag:
                to_remote_peer(ABORT, {})

            self.abort_flag = True;
        
    def tokenize(self, output: list[str]) -> None:
        ...
    
    def is_aborting(self) -> bool: 
        with self._lock:
            return self.abort_flag


class BaseQueryProxy(BaseProxy): ...