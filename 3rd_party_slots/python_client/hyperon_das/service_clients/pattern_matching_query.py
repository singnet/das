import threading

from hyperon_das.distributed_algorithm_node.bus_node import BusCommand
from hyperon_das.logger import log
from hyperon_das.query_answer import QueryAnswer
from hyperon_das.service_clients.base import BaseQueryProxy


class PatternMatchingQueryProxy(BaseQueryProxy):
    """Proxy for pattern matching queries, managing execution and results."""

    COUNT = "count"
    POSITIVE_IMPORTANCE_FLAG = "positive_importance_flag"
    COUNT_FLAG = "count_flag"

    def __init__(self, tokens: list[str], context: str = "") -> None:
        super().__init__(tokens, context)
        self._lock = threading.Lock()
        self.parameters[self.POSITIVE_IMPORTANCE_FLAG] = False
        self.parameters[self.COUNT_FLAG] = False
        self.command = BusCommand.PATTERN_MATCHING_QUERY
        log.debug(f"Creating PatternMatchingQueryProxy with context: <{context}>")

    def pop(self) -> QueryAnswer | None:
        """Pops and returns the next item from the answer queue, if available."""
        with self._lock:
            if self.parameters[self.COUNT_FLAG]:
                log.error("Can't pop QueryAnswers from count_only queries.")
                return QueryAnswer()
            else:
                return super().pop()

    def set_parameter(self, key: str, value: str | int | float | bool) -> None:
        self.parameters[key] = value

    def pack_command_line_args(self) -> None:
        super().tokenize(self.args)

    def process_message(self, msg: list[str]) -> None:
        """Processes received messages, updating the answer queue or flags."""
        log.debug(f"Processing message: {msg}")
        log.debug(f"Message length: {len(msg)}")
        with self._lock:
            for token in msg:
                log.debug(f"Processing token: {token}")
                if super().process_message(token):
                    break
                elif token in [self.COUNT, self.ANSWER_BUNDLE]:
                    continue
                else:
                    log.debug(f"Adding token to answer queue: {token}")
                    self.answer_count += 1
                    query_answer = QueryAnswer()
                    query_answer.untokenize(token)
                    self.answer_queue.put(query_answer)
