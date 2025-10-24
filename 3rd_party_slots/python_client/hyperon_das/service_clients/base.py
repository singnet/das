import threading
from hyperon_das.commons.properties import Properties
from hyperon_das.service_bus.proxy import BusCommandProxy


class BaseProxy(BusCommandProxy):
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
                self.to_remote_peer(ABORT, {})

            self.abort_flag = True;
        
    def tokenize(self, output: list[str]) -> None:
        parameters_tokens = self.parameters.tokenize()
        parameters_tokens.insert(0, str(len(parameters_tokens)))
        output[0:0] = parameters_tokens

    def is_aborting(self) -> bool: 
        with self._lock:
            return self.abort_flag

class BaseQueryProxy(BaseProxy):
    def __init__(self, tokens: list[str], context: str) -> None:
        self.answer_count = 0
        self.parameters[UNIQUE_ASSIGNMENT_FLAG] = False
        self.parameters[ATTENTION_UPDATE_FLAG] = False
        self.parameters[MAX_BUNDLE_SIZE] = 1000
        self.parameters[MAX_ANSWERS] = 0
        self.parameters[USE_LINK_TEMPLATE_CACHE] = False
        self.parameters[POPULATE_METTA_MAPPING] = False
        self.parameters[USE_METTA_AS_QUERY_TOKENS] = False
        self.context = context
        self.query_tokens.extend(tokens)
    
    def 



