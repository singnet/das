import time

from threading import Thread, Lock, Event
from queue import Queue, Full, Empty

from hyperon_das.cache.attention_broker_gateway import AttentionBrokerGateway
from hyperon_das.das_node.query_answer import QueryAnswer
from hyperon_das.das_node.das_node import DASNode
from hyperon_das.das_node.remote_iterator import RemoteIterator
from hyperon_das_atomdb.adapters import RedisMongoDB

from fitness_functions import handle_fitness_function
from selection_methods import handle_selection_method, SelectionMethodType
from utils import Parameters, parse_file


MAX_CORRELATIONS_WITHOUT_STIMULATE = 100


class QueryOptimizerIterator:
    """
    Iterator that optimizes queries by sampling, evaluating, selecting, and updating query answers.

    This class runs an optimization loop in a separate thread, where it:
      - Samples a population of QueryAnswer objects.
      - Evaluates each using a configurable fitness function.
      - Selects the best individuals via a selection method.
      - Updates an attention broker with correlated handle data.

    The iterator interface yields the best query answers once optimization completes.
    """
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
        """
        Retrieves the next available QueryAnswer from the queue.
        Blocks until an answer is available or the optimization has stopped.
        """
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
        """
        Returns the next QueryAnswer if available, or None otherwise.
        """
        if not self.is_empty():
            try:
                return self.__next__()
            except StopIteration:
                return

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

    def _producer_optimizer(self) -> None:
        """
        Runs the main optimization loop in a separate thread.

        For each generation, it samples a population, evaluates them,
        selects the best individuals, and updates the attention broker.
        After processing all generations, it fills the query answers queue and signals termination.
        """
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
        """
        Samples a population of QueryAnswer objects using a remote iterator.
        """
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
        """
        Executes a query using DASNode and returns a RemoteIterator over QueryAnswer objects.
        """
        das_node_client = DASNode(self.params.node_id, self.params.das_node_server_id)
        return das_node_client.pattern_matcher_query(query_tokens)

    def _evaluate(self, query_answer: QueryAnswer) -> tuple[QueryAnswer, float]:
        """
        Evaluates a QueryAnswer using a configured fitness function.

        Returns:
            A tuple of (QueryAnswer, fitness_value).
        """
        fitness_function = handle_fitness_function(self.params.fitness_function)
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

        single_answer = set()
        joint_answer = {}
        correlated_count = 0

        for query_answer in individuals:
            execution_stack = [handle for handle in query_answer.handles]

            while execution_stack:
                handle = execution_stack.pop()
                single_answer.add(handle)
                joint_answer[handle] = joint_answer.get(handle, 0) + 1
                try:
                    targets = self.atom_db.get_link_targets(handle)
                    for target_handle in targets:
                        execution_stack.append(target_handle)
                except ValueError:
                    pass

            handle_list = []
            # handle_list.context = self.params.context
            for h in single_answer:
                handle_list.append(h)

            single_answer.clear()
            attention_broker.correlate(handle_list)
            correlated_count += 1

            if correlated_count == MAX_CORRELATIONS_WITHOUT_STIMULATE:
                correlated_count = 0
                self._attention_broker_stimulate(attention_broker, joint_answer)
                joint_answer.clear()

        if correlated_count > 0:
            self._attention_broker_stimulate(attention_broker, joint_answer)

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

    def _select_best_query_answers(self) -> None:
        """
        Fills the best_query_answers queue by selecting QueryAnswers from a remote iterator.
        """
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
        """
        Loads and validates configuration parameters from the specified file.

        Raises:
            ValueError: If the number of individuals selected for attention update exceeds population size.
        """
        args = parse_file(config_file)
        params = Parameters(*args)
        if params.qtd_selected_for_attention_update > params.population_size:
            raise ValueError("The number of selected individuals for attention update cannot be greater than the population size.")
        return params

    def _shutdown_optimizer(self) -> None:
        """
        Signals the optimizer thread to shut down gracefully.
        """
        if self.producer_optimizer.is_alive():
            self.stop_event.set()
