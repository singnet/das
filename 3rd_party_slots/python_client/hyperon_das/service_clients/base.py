import threading
from queue import Empty, Queue

from hyperon_das.commons.properties import Properties
from hyperon_das.query_answer import QueryAnswer
from hyperon_das.service_bus.proxy import BusCommandProxy


class BaseProxy(BusCommandProxy):
    ABORT = "abort"
    FINISHED = "finished"

    def __init__(self):
        super().__init__()
        self._lock_base_proxy = threading.Lock()
        self.abort_flag = False
        self.command_finished_flag = False
        self.parameters = Properties()

    def finished(self) -> bool:
        with self._lock_base_proxy:
            return self.abort_flag or self.command_finished_flag

    def abort(self) -> None:
        with self._lock_base_proxy:
            if not self.command_finished_flag:
                self.to_remote_peer(self.ABORT, {})

            self.abort_flag = True

    def tokenize(self, output: list[str]) -> None:
        parameters_tokens = self.parameters.tokenize()
        parameters_tokens.insert(0, str(len(parameters_tokens)))
        output[0:0] = parameters_tokens

    def is_aborting(self) -> bool:
        with self._lock_base_proxy:
            return self.abort_flag

    def process_message(self, msg: list[str]) -> bool:
        with self._lock_base_proxy:
            if msg == self.FINISHED:
                if not self.abort_flag:
                    self.command_finished_flag = True
                    return True
            elif msg == self.ABORT:
                self.abort_flag = True
                return True
            return False


class BaseQueryProxy(BaseProxy):
    ANSWER_BUNDLE = "answer_bundle"
    ABORT = "abort"
    FINISHED = "finished"

    UNIQUE_ASSIGNMENT_FLAG = "unique_assignment_flag"
    ATTENTION_UPDATE_FLAG = "attention_update_flag"
    MAX_BUNDLE_SIZE = "max_bundle_size"
    MAX_ANSWERS = "max_answers"
    USE_LINK_TEMPLATE_CACHE = "use_link_template_cache"
    POPULATE_METTA_MAPPING = "populate_metta_mapping"
    USE_METTA_AS_QUERY_TOKENS = "user_metta_as_query_tokens"

    def __init__(self, tokens: list[str], context: str = "") -> None:
        super().__init__()
        self._lock_base_query_proxy = threading.Lock()
        self.answer_queue = Queue()
        self.answer_count = 0
        self.parameters[self.UNIQUE_ASSIGNMENT_FLAG] = False
        self.parameters[self.ATTENTION_UPDATE_FLAG] = False
        self.parameters[self.MAX_BUNDLE_SIZE] = 1000
        self.parameters[self.MAX_ANSWERS] = 0
        self.parameters[self.USE_LINK_TEMPLATE_CACHE] = False
        self.parameters[self.POPULATE_METTA_MAPPING] = False
        self.parameters[self.USE_METTA_AS_QUERY_TOKENS] = False
        self.context = context
        self.query_tokens = tokens

    def pop(self) -> QueryAnswer | None:
        with self._lock_base_query_proxy:
            if self.is_aborting():
                return QueryAnswer()
            else:
                try:
                    return self.answer_queue.get(block=False)
                except Empty:
                    return None

    def get_context(self) -> str:
        with self._lock_base_query_proxy:
            return self.context

    def get_count(self) -> int:
        with self._lock_base_query_proxy:
            return self.answer_count

    def set_count(self, count: int) -> None:
        with self._lock_base_query_proxy:
            self.answer_count = count

    def tokenize(self, output: list[str]) -> None:
        output[0:0] = self.query_tokens
        output.insert(0, str(len(self.query_tokens)))
        output.insert(0, self.get_context())
        super().tokenize(output)

    def finished(self) -> bool:
        with self._lock_base_query_proxy:
            return self.is_aborting() or (super().finished() and self.answer_queue.empty())

    def process_message(self, msg: list[str]) -> bool:
        with self._lock_base_query_proxy:
            if super().process_message(msg):
                return True
            return False
