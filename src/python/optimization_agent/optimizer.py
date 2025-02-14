import time

from threading import Thread, Lock, Event
from queue import Queue, Full, Empty

from hyperon_das.cache.attention_broker_gateway import AttentionBrokerGateway
from hyperon_das.das_node.query_answer import QueryAnswer
from hyperon_das.das_node.das_node import DASNode
from hyperon_das.das_node.remote_iterator import RemoteIterator
from hyperon_das_atomdb.adapters import RedisMongoDB

from fitness_functions import handle_fitness_function
from utils import Parameters, parse_file


class QueryOptimizerIterator:
    def __init__(self, config_file: str, query_tokens: list[str]) -> None:
        self.params = self._load_config(config_file)
        self.atom_db = self._connect_atom_db()
        self.query = query_tokens
        self.best_query_answers = Queue(maxsize=self.params.max_query_answers)
        self.generation = 1
        self.stop_event = Event()
        self.lock = Lock()
        self.producer_optimizer = Thread(target=self._producer_optimizer, daemon=True)
        self.producer_optimizer.start()

    def __iter__(self):
        return self

    def __next__(self):
        while True:
            try:
                return self.best_query_answers.get(timeout=1)
            except Empty:
                if self.stop_event.is_set() and self.best_query_answers.empty():
                    raise StopIteration
                time.sleep(0.1)

    def is_empty(self) -> bool:
        return self.best_query_answers.empty()

    def pop(self) -> QueryAnswer:
        if not self.is_empty():
            try:
                return self.__next__()
            except StopIteration:
                return

    def _connect_atom_db(self) -> RedisMongoDB:
        try:
            return RedisMongoDB(
                mongo_hostname=self.params.mongo_hostname,
                mongo_port=self.params.mongo_port,
                mongo_username=self.params.mongo_username,
                mongo_password=self.params.mongo_password,
                redis_hostname=self.params.redis_hostname,
                redis_port=self.params.redis_port,
                redis_cluster=self.params.redis_cluster,
                redis_ssl=self.params.redis_ssl,
            )
        except Exception as e:
            print(f"Error: {e}")
            raise e

    def _producer_optimizer(self) -> None:
        while self.generation <= self.params.max_generations:
            with self.lock:
                population = self._sample_population(self.query)

            evaluated_individuals = [self._evaluate(individual) for individual in population]
            selected_individuals = self._select_best_individuals(evaluated_individuals)
            self._update_attention_broker(selected_individuals)
            self.generation += 1

            time.sleep(0.5)

        self._select_best_query_answers()
        self.stop_event.set()

    def _sample_population(self, query_tokens: list[str]) -> list[QueryAnswer]:
        result = []
        try:
            with self._query(query_tokens) as remote_iterator:
                while (len(result) < self.params.population_size and not remote_iterator.finished()):
                    if (qa := remote_iterator.pop()):
                        result.append(qa)
                    else:
                        time.sleep(0.1)
            time.sleep(0.5)
        except Exception as e:
            print(f"Population sampling failed: {e}")
        return result

    def _query(self, query_tokens: list[str]) -> RemoteIterator:
        das_node_client = DASNode(self.params.node_id, self.params.das_node_server_id)
        return das_node_client.pattern_matcher_query(query_tokens, "python", True)

    def _evaluate(self, query_answer: QueryAnswer) -> tuple[QueryAnswer, float]:
        fitness_function = handle_fitness_function(self.params.fitness_function)
        fitness_value = fitness_function(self.atom_db, query_answer)
        return (query_answer, fitness_value)

    def _select_best_individuals(self, evaluated_individuals: list[tuple[QueryAnswer, float]]) -> list[tuple[QueryAnswer, float]]:
        selected_individuals = []
        evaluated_sorted = sorted(evaluated_individuals, key=lambda x: x[1], reverse=True)
        while evaluated_sorted and len(selected_individuals) < self.params.qtd_selected_for_attention_update:
            try:
                individual, _ = evaluated_sorted.pop(0)
                selected_individuals.append(individual)
            except IndexError as e:
                print(f"Error: {e}")
        return selected_individuals

    def _update_attention_broker(self, individuals: list[tuple[QueryAnswer, float]]) -> None:
        # attention_broker = AttentionBrokerGateway({
        #     'attention_broker_hostname': "localhost",
        #     'attention_broker_port': 37007
        # })
        # attention_broker.ping()
        # attention_broker.stimulate({})
        print("WIP - Update attentionBroker")
        return

    def _select_best_query_answers(self) -> None:
        with self.lock:
            with self._query(self.query) as remote_iterator:
                while (self.best_query_answers.qsize() < self.params.max_query_answers and not remote_iterator.finished()):
                    if (qa := remote_iterator.pop()):
                        try:
                            self.best_query_answers.put(qa, timeout=1)
                        except Full:
                            print("Queue Full")
                    else:
                        time.sleep(0.1)
            time.sleep(0.5)

    def _load_config(self, config_file: str) -> Parameters:
        args = parse_file(config_file)
        return Parameters(*args)

    def _shutdown_optimizer(self) -> None:
        if self.producer_optimizer.is_alive():
            self.stop_event.set()
