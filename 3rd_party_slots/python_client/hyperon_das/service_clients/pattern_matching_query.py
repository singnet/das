import threading
from queue import Queue, Empty

from hyperon_das.distributed_algorithm_node.bus_node import BusCommand
from hyperon_das.service_bus.proxy import BaseCommandProxy
from hyperon_das.logger import log
from hyperon_das.query_answer import QueryAnswer
from hyperon_das.commons.properties import Properties, ATTENTION_UPDATE_FLAG, MAX_BUNDLE_SIZE, UNIQUE_ASSIGNMENT_FLAG, POSITIVE_IMPORTANCE_FLAG, COUNT_FLAG, POPULATE_METTA_MAPPING


class PatternMatchingQueryProxy(BaseCommandProxy):
    """Proxy for pattern matching queries, managing execution and results."""

    ABORT = "abort"
    ANSWER_BUNDLE = "answer_bundle"
    COUNT = "count"
    FINISHED = "finished"

    def __init__(
        self,
        tokens: list[str],
        context: str = "",
        update_attention_broker: bool = False,
        positive_importance: bool = False,
        populate_metta_mapping: bool = False,
        unique_assignment: bool = False,
        count_only: bool = False
    ) -> None:
        properties = Properties()
        properties.insert(ATTENTION_UPDATE_FLAG, update_attention_broker)
        properties.insert(POSITIVE_IMPORTANCE_FLAG, positive_importance)
        properties.insert(POPULATE_METTA_MAPPING, populate_metta_mapping)
        properties.insert(UNIQUE_ASSIGNMENT_FLAG, unique_assignment)
        properties.insert(COUNT_FLAG, count_only)
        properties.insert(MAX_BUNDLE_SIZE, 1000)

        args = properties.tokenize()
        args.append(context)
        args.append(str(len(tokens)))
        args.extend(tokens)

        log.debug(f"Creating PatternMatchingQueryProxy with args: {args}")

        super().__init__(command=BusCommand.PATTERN_MATCHING_QUERY, args=args)

        self.answer_queue = Queue()
        self._lock = threading.Lock()
        self.answer_flow_finished = False
        self.abort_flag = False
        self.answer_count = 0
        self.count_flag = count_only

    def finished(self) -> bool:
        """Checks if the answers is finished or aborted."""
        with self._lock:
            return (
                self.abort_flag or (
                    self.answer_flow_finished and (self.count_flag or self.answer_queue.empty())
                )
            )

    def pop(self) -> QueryAnswer | None:
        """Pops and returns the next item from the answer queue, if available."""
        with self._lock:
            if self.count_flag:
                log.warning("Can't pop QueryAnswers from count_only queries.")
                return None
            if self.abort_flag:
                log.warning(f"abort_flag: {self.abort_flag}")
                return None
            try:
                return self.answer_queue.get(block=False)
            except Empty:
                return None

    def process_message(self, msg: list[str]) -> None:
        """Processes received messages, updating the answer queue or flags."""
        log.debug(f"Processing message: {msg}")
        log.debug(f"Message length: {len(msg)}")
        with self._lock:
            for token in msg:
                log.debug(f"Processing token: {token}")
                if token == self.FINISHED:
                    if not self.abort_flag:
                        self.answer_flow_finished = True
                    break
                elif token == self.ABORT:
                    self.abort_flag = True
                    break
                elif token in [self.ANSWER_BUNDLE, self.COUNT]:
                    continue
                else:
                    log.debug(f"Adding token to answer queue: {token}")
                    self.answer_count += 1
                    query_answer = QueryAnswer()
                    query_answer.untokenize(token)
                    self.answer_queue.put(query_answer)