import threading
import time
from typing import List, Optional

from hyperon_das_node import DistributedAlgorithmNode, LeadershipBrokerType, Message, MessageBrokerType

from evolution.das_node.query_answer import QueryAnswer
from evolution.das_node.shared_queue import SharedQueue


class QueryNode(DistributedAlgorithmNode):
    QUERY_ANSWER_FLOW_COMMAND = "query_answer_flow"
    QUERY_ANSWER_TOKENS_FLOW_COMMAND = "query_answer_tokens_flow"
    QUERY_ANSWERS_FINISHED_COMMAND = "query_answers_finished"

    def __init__(self, node_id: str, is_server: bool, messaging_backend: MessageBrokerType):
        super().__init__(node_id, LeadershipBrokerType.SINGLE_MASTER_SERVER, messaging_backend)
        self.is_server = is_server
        self.query_answer_processor: Optional[threading.Thread] = None
        self.query_answers_finished_flag = False
        self.shutdown_flag = False
        self.shutdown_flag_mutex = threading.Lock()
        self.query_answers_finished_flag_mutex = threading.Lock()

        self.requires_serialization = messaging_backend != MessageBrokerType.RAM
        self.query_answer_queue = SharedQueue()

    def __del__(self):
        self.graceful_shutdown()

    def message_factory(self, command: str, args: List[str]) -> Optional[Message]:
        message = super().message_factory(command, args)
        if message:
            return message
        if command == QueryNode.QUERY_ANSWER_FLOW_COMMAND:
            return QueryAnswerFlow(command, args)
        elif command == QueryNode.QUERY_ANSWER_TOKENS_FLOW_COMMAND:
            return QueryAnswerTokensFlow(command, args)
        elif command == QueryNode.QUERY_ANSWERS_FINISHED_COMMAND:
            return QueryAnswersFinished(command, args)
        return None

    def graceful_shutdown(self):
        if self.is_shutting_down():
            return
        with self.shutdown_flag_mutex:
            self.shutdown_flag = True
        if self.query_answer_processor:
            self.query_answer_processor.join()
            self.query_answer_processor = None

    def is_shutting_down(self) -> bool:
        with self.shutdown_flag_mutex:
            return self.shutdown_flag

    def query_answers_finished(self):
        with self.query_answers_finished_flag_mutex:
            self.query_answers_finished_flag = True

    def is_query_answers_finished(self) -> bool:
        with self.query_answers_finished_flag_mutex:
            return self.query_answers_finished_flag

    def add_query_answer(self, query_answer: QueryAnswer):
        if self.is_query_answers_finished():
            raise ValueError("Invalid addition of new query answer.")
        self.query_answer_queue.enqueue(query_answer)

    def pop_query_answer(self) -> QueryAnswer:
        return self.query_answer_queue.dequeue()

    def is_query_answers_empty(self) -> bool:
        return self.query_answer_queue.empty()

    def query_answer_processor_method(self):
        raise NotImplementedError


class QueryNodeServer(QueryNode):
    def __init__(self, node_id: str, messaging_backend: MessageBrokerType = MessageBrokerType.RAM):
        super().__init__(node_id, is_server=True, messaging_backend=messaging_backend)
        self.join_network()
        self.query_answer_processor = threading.Thread(target=self.query_answer_processor_method)
        self.query_answer_processor.start()

    def __del__(self):
        self.graceful_shutdown()

    def node_joined_network(self, node_id: str):
        self.add_peer(node_id)

    def cast_leadership_vote(self) -> str:
        return self.node_id()

    def query_answer_processor_method(self):
        while not self.is_shutting_down():
            time.sleep(0.5)  # Sleep to avoid high CPU usage


class QueryAnswerFlow(Message):
    def __init__(self, command: str, args: List[str]):
        super().__init__()
        self.query_answers = [QueryAnswer(pointer) for pointer in args]

    def act(self, arg, *args, **kwargs):
        query_node = arg
        for query_answer in self.query_answers:
            query_node.add_query_answer(query_answer)


class QueryAnswerTokensFlow(Message):
    def __init__(self, command: str, args: List[str]):
        super().__init__()
        self.query_answers_tokens = args

    def act(self, arg, *args, **kwargs):
        query_node = arg
        for tokens in self.query_answers_tokens:
            query_answer = QueryAnswer()
            query_answer.untokenize(tokens)
            query_node.add_query_answer(query_answer)


class QueryAnswersFinished(Message):
    def __init__(self, command: str, args: List[str]):
        super().__init__()

    def act(self, arg, *args, **kwargs):
        query_node = arg
        query_node.query_answers_finished()
