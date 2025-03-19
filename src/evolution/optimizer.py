"""
Non distributed version
"""
import contextlib
import json
import time
import sys

from threading import Thread, Event, Lock
from queue import Queue, Full
from typing import Iterator
from concurrent.futures import ThreadPoolExecutor
from collections import defaultdict

from hyperon_das.cache.attention_broker_gateway import AttentionBrokerGateway
from hyperon_das_atomdb.adapters import RedisMongoDB

from evolution.fitness_functions import FitnessFunctions
from evolution.selection_methods import handle_selection_method, SelectionMethodType
from evolution.utils import Parameters, parse_file, SuppressCppOutput
from evolution.node import EvolutionNode, NodeIdFactory

# NOTE: This module, das_node, is a Python implementation of DASNode,
# used to develop and test the optimization algorithm.
# However, it is necessary to create bindings for the components already implemented in C++
# and integrate them into this implementation.
# Currently, I am using the implementation from this PR:
from evolution.das_node.query_answer import QueryAnswer
from evolution.das_node.das_node import DASNode

MAX_CORRELATIONS_WITHOUT_STIMULATE = 1000


class QueryOptimizerAgent:
    def __init__(self, config_file: str) -> None:
        self.params = self._load_config(config_file)
        self.node_id_factory = NodeIdFactory(ip="localhost")
        self.evolution_node_server = EvolutionNode(node_id=self.params.evolution_server_id)

    def run_server(self):
        while True:
            if request := self.evolution_node_server.pop_request():
                answers = self._process(request['data'])
                self._send_message(request['senders'], answers)
            time.sleep(0.5)

    def optimize(self, query_tokens: list[str] | str) -> Iterator:
        if isinstance(query_tokens, str):
            query_tokens = query_tokens.split(' ')
        return QueryOptimizerIterator(
            query_tokens,
            **self.params.__dict__
        )

    def _load_config(self, config_file: str) -> Parameters:
        """
        Loads and validates configuration parameters from the specified file.

        Raises:
            ValueError: If the number of individuals selected for attention update exceeds population size.
        """
        config = parse_file(config_file)
        params = Parameters(**config)
        if params.qtd_selected_for_attention_update > params.population_size:
            raise ValueError("The number of selected individuals for attention update cannot be greater than the population size.")
        return params

    def _process(self, query_tokens: list[str] | str) -> list[str]:
        iterator = self.optimize(query_tokens)
        return [qa.to_string() for qa in iterator]

    def _send_message(self, senders: list[str], answers: list[str]) -> None:
        for sender in senders:
            self.evolution_node_client = EvolutionNode(
                server_id=sender,
                node_id_factory=self.node_id_factory
            )
            self.evolution_node_client.send("evolution_finished", ["END"], sender)


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
    def __init__(
        self,
        query_tokens: list[str],
        max_query_answers: int,
        max_generations: int,
        population_size: int,
        qtd_selected_for_attention_update: int,
        selection_method: str,
        fitness_function: str,
        attention_broker_server_id: str,
        query_agent_node_id: str,
        query_agent_server_id: str,
        mongo_hostname: str,
        mongo_port: int,
        mongo_username: str,
        mongo_password: str,
        redis_hostname: str,
        redis_port: int,
        redis_cluster: bool,
        redis_ssl: bool,
        **kwargs
    ) -> None:
        self.atom_db = self._connect_atom_db(
            mongo_hostname, mongo_port, mongo_username, mongo_password,
            redis_hostname, redis_port, redis_cluster, redis_ssl
        )

        with SuppressCppOutput():
            self.query_agent = DASNode(query_agent_node_id, query_agent_server_id)

        self.query_tokens = query_tokens
        self.max_query_answers = max_query_answers
        self.max_generations = max_generations
        self.population_size = population_size
        self.qtd_selected_for_attention_update = qtd_selected_for_attention_update
        self.selection_method = selection_method
        self.fitness_function = fitness_function
        self.attention_broker_server_id = attention_broker_server_id

        self.best_query_answers = Queue(maxsize=self.max_query_answers)
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
                if not self.best_query_answers.empty():
                    return self.best_query_answers.get()
                elif self.is_empty():
                    self._shutdown_optimizer()
                    raise StopIteration
            time.sleep(0.1)

    def is_empty(self) -> bool:
        return bool(self.best_query_answers.empty() and self.producer_finished.is_set())

    def _connect_atom_db(
        self,
        mongo_hostname: str,
        mongo_port: int,
        mongo_username: str,
        mongo_password: str,
        redis_hostname: str,
        redis_port: int,
        redis_cluster: bool,
        redis_ssl: bool
    ) -> RedisMongoDB:
        """
        Establishes a connection to the atom database using configuration parameters.
        """
        try:
            return RedisMongoDB(
                mongo_hostname=mongo_hostname,
                mongo_port=mongo_port,
                mongo_username=mongo_username,
                mongo_password=mongo_password,
                redis_hostname=redis_hostname,
                redis_port=redis_port,
                redis_cluster=redis_cluster,
                redis_ssl=redis_ssl,
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
        while self.generation <= self.max_generations:
            sys.stdout.write(f"\rProcessing generation {self.generation}/{self.max_generations}")
            sys.stdout.flush()

            population = self._sample_population()

            if not population:
                self.producer_finished.set()
                raise RuntimeError("Population sampling failed: query response returned nothing.")

            with ThreadPoolExecutor(max_workers=10) as executor:
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
                remote_iterator = self.query_agent.pattern_matcher_query(self.query_tokens)
            while (len(result) < self.population_size and not remote_iterator.finished()):
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
        fitness_function = FitnessFunctions.get(self.fitness_function)
        with self.atom_db_mutex:
            fitness_value = fitness_function(self.atom_db, query_answer.handles)
        return (query_answer, fitness_value)

    def _select_best_individuals(self, evaluated_individuals: list[tuple[QueryAnswer, float]]) -> list[tuple[QueryAnswer, float]]:
        """
        Selects the best individuals from evaluated QueryAnswer tuples using the configured selection method.
        """
        method = SelectionMethodType(self.selection_method)
        selection_method = handle_selection_method(method)
        return selection_method(evaluated_individuals, self.qtd_selected_for_attention_update)

    def _update_attention_broker(self, individuals: list[QueryAnswer]) -> None:
        """
        Updates the attention broker by correlating handles extracted from selected QueryAnswers.

        Handles are recursively collected from each QueryAnswer, and after processing
        a defined number of correlations, the attention broker is stimulated with aggregated data.
        """
        host, port = self.attention_broker_server_id.split(":")
        attention_broker = AttentionBrokerGateway({
            'attention_broker_hostname': host,
            'attention_broker_port': port
        })

        correlated_count = 0
        joint_answer_global = defaultdict(int)

        for individual in individuals:
            single_answer, joint_answer = self._process_individual(individual)

            # How to send the context?
            # handle_list = []
            # handle_list.context = self.context
            # for h in single_answer:
            #     handle_list.append(h)

            attention_broker.correlate(single_answer)

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
        # handle_count.context = self.context
        attention_broker.stimulate(handle_count)
        time.sleep(0.1)

    def _select_best_query_answers(self) -> None:
        """
        Fills the best_query_answers queue by selecting QueryAnswers from a remote iterator.
        """
        with SuppressCppOutput():
            remote_iterator = self.query_agent.pattern_matcher_query(self.query_tokens)
        while (self.best_query_answers.qsize() < self.max_query_answers and not remote_iterator.finished()):
            if (qa := remote_iterator.pop()):
                with contextlib.suppress(Full):
                    self.best_query_answers.put(qa, timeout=1)
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
