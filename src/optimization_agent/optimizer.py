import time

from threading import Thread, Event, Lock
from queue import Queue, Full
from typing import Iterator
from collections import defaultdict

from hyperon_das.cache.attention_broker_gateway import AttentionBrokerGateway
from hyperon_das_atomdb.adapters import RedisMongoDB
from concurrent.futures import ThreadPoolExecutor

from fitness_functions import handle_fitness_function
from selection_methods import handle_selection_method, SelectionMethodType
from utils import Parameters, parse_file, profile, SuppressCppOutput

# NOTE: This module, das_node, is a Python implementation of DASNode,
# used to develop and test the optimization algorithm.
# However, it is necessary to create bindings for the components already implemented in C++
# and integrate them into this implementation.
# Currently, I am using the implementation from this PR:
from das_node.query_answer import QueryAnswer
from das_node.das_node import DASNode

MAX_CORRELATIONS_WITHOUT_STIMULATE = 1000


class SharedQueue:
    def __init__(self, maxsize):
        self.queue = Queue(maxsize)

    def enqueue(self, request):
        try:
            self.queue.put(request, timeout=1)
        except Full:
            print("Queue Full")

    def dequeue(self):
        if not self.empty():
            return self.queue.get()
        return None

    def empty(self) -> bool:
        return self.queue.empty()

    def size(self) -> int:
        return self.queue.qsize()


class QueryOptimizerAgent:
    def __init__(self, config_file: str) -> None:
        self.params = self._load_config(config_file)

    def optimize(self, query_tokens: list[str]) -> Iterator:
        iterator = QueryOptimizerIterator(query_tokens, self.params)
        return iterator

    def _load_config(self, config_file: str) -> Parameters:
        """
        Loads and validates configuration parameters from the specified file.

        Raises:
            ValueError: If the number of individuals selected for attention update exceeds population size.
        """
        config = parse_file(config_file)
        params = Parameters(*config.values())
        if params.qtd_selected_for_attention_update > params.population_size:
            raise ValueError("The number of selected individuals for attention update cannot be greater than the population size.")
        return params


class QueryOptimizerIterator:
    """
    Iterator that optimizes queries by sampling, evaluating, selecting, and updating query answers.

    This class runs an optimization loop, where it:
      - Samples a population of QueryAnswer objects.
      - Evaluates each using a configurable fitness function.
      - Selects the best individuals via a selection method.
      - Updates an attention broker with correlated handle data.

    The iterator interface yields the best query answers once optimization completes.
    """
    def __init__(self, query_tokens: list[str], parameters: Parameters, atom_db: str = None, das_node: DASNode = None) -> None:
        self.params = parameters
        self.atom_db = atom_db or self._connect_atom_db()
        with SuppressCppOutput():
            self.das_node_client = das_node or DASNode(self.params.node_id, self.params.das_node_server_id)
        self.query = query_tokens
        self.best_query_answers = SharedQueue(maxsize=self.params.max_query_answers)
        self.generation = 1
        self.producer_finished = Event()
        self.atom_db_mutex = Lock()
        self._producer_thread = Thread(target=self._producer)
        self._producer_thread.start()

    def __iter__(self):
        return self

    def __next__(self):
        """
        Retrieves the next available QueryAnswer from the queue.
        """
        while True:
            if self.producer_finished.is_set():
                if (query_answer := self.best_query_answers.dequeue()):
                    return query_answer
                elif self.is_empty():
                    self._shutdown_optimizer()
                    raise StopIteration
            time.sleep(0.1)

    def is_empty(self) -> bool:
        return bool(self.best_query_answers.empty() and self.producer_finished.is_set())

    def _connect_atom_db(self) -> RedisMongoDB:
        """
        Establishes a connection to the atom database using configuration parameters.
        """
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

    def _producer(self) -> None:
        """
        Runs the main optimization loop in a separate thread.

        For each generation, it samples a population, evaluates them,
        selects the best individuals, and updates the attention broker.
        After processing all generations, it fills the query answers queue and signals termination.
        """
        while self.generation <= self.params.max_generations:
            population = self._sample_population()

            if not population:
                raise RuntimeError("Population sampling failed: query response returned nothing.")

            with ThreadPoolExecutor() as executor:
                futures = [executor.submit(self._evaluate, ind) for ind in population]
                evaluated_individuals = [f.result() for f in futures]

            selected_individuals = self._select_best_individuals(evaluated_individuals)
            self._update_attention_broker(selected_individuals)
            self.generation += 1
            time.sleep(0.1)

        self._select_best_query_answers()
        self.producer_finished.set()

    def _sample_population(self) -> list[QueryAnswer]:
        """
        Samples a population of QueryAnswer objects using a remote iterator.
        """
        result = []
        try:     
            with SuppressCppOutput():
                remote_iterator = self.das_node_client.pattern_matcher_query(self.query)
            while (len(result) < self.params.population_size and not remote_iterator.finished()):
                if (qa := remote_iterator.pop()):
                    result.append(qa)
                else:
                    time.sleep(0.1)
            time.sleep(0.5)
        except Exception as e:
            print(f"Population sampling failed: {e}")
        return result

    def _evaluate(self, query_answer: QueryAnswer) -> tuple[QueryAnswer, float]:
        """
        Evaluates a QueryAnswer using a configured fitness function.

        Returns:
            A tuple of (QueryAnswer, fitness_value).
        """
        fitness_function = handle_fitness_function(self.params.fitness_function)
        with self.atom_db_mutex:
            fitness_value = fitness_function(self.atom_db, query_answer)
        return (query_answer, fitness_value)

    def _select_best_individuals(self, evaluated_individuals: list[tuple[QueryAnswer, float]]) -> list[tuple[QueryAnswer, float]]:
        """
        Selects the best individuals from evaluated QueryAnswer tuples using the configured selection method.
        """
        method = SelectionMethodType(self.params.selection_method)
        selection_method = handle_selection_method(method)
        return selection_method(evaluated_individuals, self.params.qtd_selected_for_attention_update)

    def _update_attention_broker(self, individuals: list[QueryAnswer]) -> None:
        """
        Updates the attention broker by correlating handles extracted from selected QueryAnswers.

        Handles are recursively collected from each QueryAnswer, and after processing
        a defined number of correlations, the attention broker is stimulated with aggregated data.
        """
        host, port = self.params.attention_broker_server.split(":")
        attention_broker = AttentionBrokerGateway({
            'attention_broker_hostname': host,
            'attention_broker_port': port
        })

        joint_answer_global = defaultdict(int)
        correlated_count = 0

        with ThreadPoolExecutor() as executor:
            futures = [executor.submit(self._process_individual, ind) for ind in individuals]

            for future in futures:
                single_answer, joint_answer = future.result()

                attention_broker.correlate(single_answer)
                time.sleep(0.01)

                for handle, count in joint_answer.items():
                    joint_answer_global[handle] += count

                correlated_count += 1

                if correlated_count >= MAX_CORRELATIONS_WITHOUT_STIMULATE:
                    self._attention_broker_stimulate(attention_broker, joint_answer_global)
                    joint_answer_global.clear()
                    correlated_count = 0

        if correlated_count > 0:
            self._attention_broker_stimulate(attention_broker, joint_answer_global)

    def _process_individual(self, query_answer: QueryAnswer):
        execution_stack = [handle for handle in query_answer.handles]
        single_answer = set()
        joint_answer = {}
        while execution_stack:
            handle = execution_stack.pop()
            single_answer.add(handle)
            joint_answer[handle] = joint_answer.get(handle, 0) + 1
            try:
                with self.atom_db_mutex:
                    targets = self.atom_db.get_link_targets(handle)
                for target_handle in targets:
                    execution_stack.append(target_handle)
            except ValueError:
                pass
        return single_answer, joint_answer

    def _attention_broker_stimulate(self, attention_broker, joint_answer) -> None:
        """
        Stimulates the attention broker using aggregated handle counts.

        Aggregates the counts and adds a "SUM" entry representing the total weight.
        """
        handle_count = {}
        weight_sum = 0
        for h, count in joint_answer.items():
            handle_count[h] = count
            weight_sum += count
        handle_count["SUM"] = weight_sum
        # handle_count.context = self.params.context
        attention_broker.stimulate(handle_count)
        time.sleep(0.1)

    def _select_best_query_answers(self) -> None:
        """
        Fills the best_query_answers queue by selecting QueryAnswers from a remote iterator.
        """
        with SuppressCppOutput():
            remote_iterator = self.das_node_client.pattern_matcher_query(self.query)
        while (self.best_query_answers.size() < self.params.max_query_answers and not remote_iterator.finished()):
            if (qa := remote_iterator.pop()):
                self.best_query_answers.enqueue(qa)
            else:
                time.sleep(0.1)
        time.sleep(0.5)

    def _shutdown_optimizer(self) -> None:
        """
        Signals the optimizer thread to shut down gracefully.
        """
        self.producer_finished.set()
        if self._producer_thread.is_alive():
            self._producer_thread.join(timeout=2)
