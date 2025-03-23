"""
Non distributed version
"""

import contextlib
import time
import sys

from typing import Any
from threading import Event, Lock
from queue import Queue, Full
from typing import Iterator
from concurrent.futures import ThreadPoolExecutor

from hyperon_das_atomdb.adapters import RedisMongoDB

# from hyperon_das_query_engine import DASNode, QueryAnswer
from evolution.das_node.das_node import DASNode
from evolution.das_node.query_answer import QueryAnswer

from hyperon_das_atomdb.exceptions import AtomDoesNotExist

from evolution.fitness_functions import FitnessFunctions
from evolution.selection_methods import handle_selection_method, SelectionMethodType
from evolution.utils import Parameters, parse_file, log_function_call
from evolution.node import EvolutionNode, NodeIdFactory
from evolution.attention_broker_updater import AttentionBrokerUpdater


class QueryOptimizerAgent:
    def __init__(self, config_file: str) -> None:
        self.params = self._load_config(config_file)
        self.node_id_factory = NodeIdFactory(ip="localhost")
        self.evolution_node_server = EvolutionNode(node_id=self.params.evolution_server_id)

    def run_server(self):
        sys.stdout.write("\nRunning server...")
        sys.stdout.flush()
        while True:
            if request := self.evolution_node_server.pop_request():
                sys.stdout.write("\nA message has arrived!")
                sys.stdout.flush()
                answers = self._process(request['data'])
                self._send_message(request['senders'], answers)
            time.sleep(0.5)

    def optimize(self, query_tokens: list[str] | str) -> Iterator:
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
            raise ValueError(
                "The number of selected individuals for attention update cannot be greater than the population size."
            )
        return params

    @log_function_call
    def _process(self, query_tokens: list[str] | str) -> list[str]:
        iterator = self.optimize(query_tokens)
        return [qa.to_string() for qa in iterator]

    @log_function_call
    def _send_message(self, senders: list[str], answers: list[str]) -> None:
        for sender in senders:
            self.evolution_node_client = EvolutionNode(
                server_id=sender,
                node_id_factory=self.node_id_factory
            )
            self.evolution_node_client.send("evolution_finished", ["END",], sender)


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
        query_tokens: list[str] | str,
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
        context: str = "",
        **kwargs
    ) -> None:
        self.atom_db = self._connect_atom_db(
            mongo_hostname,
            mongo_port,
            mongo_username,
            mongo_password,
            redis_hostname,
            redis_port,
            redis_cluster,
            redis_ssl,
        )
        self.attention_broker_updater = AttentionBrokerUpdater(
            address=attention_broker_server_id,
            context=context,
            atomdb=self.atom_db
        )
        self.query_agent = DASNode(query_agent_node_id, query_agent_server_id)
        self.query_tokens = self._parse_tokens(query_tokens)
        self.max_query_answers = max_query_answers
        self.max_generations = max_generations
        self.population_size = population_size
        self.qtd_selected_for_attention_update = qtd_selected_for_attention_update
        self.selection_method = selection_method
        self.fitness_function = fitness_function
        self.context = context
        self.best_query_answers = Queue(maxsize=self.max_query_answers)
        self.generation = 1
        self.producer_finished = Event()
        self.atom_db_mutex = Lock()
        self._producer()

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
        redis_ssl: bool,
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

    @log_function_call
    def _parse_tokens(self, tokens: list[str] | str) -> list[str]:
        def parse_atom(token: str, atom_db: Any) -> list[str]:
            try:
                atom = atom_db.get_atom(token)
            except AtomDoesNotExist as e:
                raise e
            if atom_db._is_document_link(atom.to_dict()):
                result = ['LINK_TEMPLATE', atom.named_type, str(len(atom.targets))]
                for target in atom.targets:
                    result.extend(parse_atom(target, atom_db))
                return result
            else:
                return ['NODE', atom.named_type, atom.name]

        parsed_tokens = []

        if isinstance(tokens, str):
            tokens = tokens.split(' ')

        token_iter = iter(tokens)
        for token in token_iter:
            if token == 'HANDLE':
                try:
                    handle_token = next(token_iter)
                    parsed_tokens.extend(parse_atom(handle_token, self.atom_db))
                except StopIteration:
                    raise ValueError("'HANDLE' Token missing corresponding value")
            else:
                parsed_tokens.append(token)
        return parsed_tokens

    def _producer(self) -> None:
        """
        Runs the main optimization loop in a separate thread.

        For each generation, it samples a population, evaluates them,
        selects the best individuals, and updates the attention broker.
        After processing all generations, it fills the query answers queue and signals termination.
        """
        while self.generation <= self.max_generations:
            sys.stdout.write(f"\ngeneration {self.generation}/{self.max_generations}")
            sys.stdout.flush()

            population = self._sample_population(limit=self.population_size)

            if not population:
                self.producer_finished.set()
                break

            with ThreadPoolExecutor(max_workers=10) as executor:
                futures = [executor.submit(self._evaluate, ind) for ind in population]
                evaluated_individuals = [f.result() for f in futures]

            selected_individuals = self._select_best_individuals(evaluated_individuals)

            try:
                for ind in selected_individuals:
                    self.attention_broker_updater.process_answer(ind)
            except Exception as e:
                sys.stdout.write(f'\nAn error occurred - details: {e}')
                sys.stdout.flush()

            self.generation += 1
            time.sleep(0.1)

        self._select_best_query_answers()
        self.producer_finished.set()

    @log_function_call
    def _sample_population(self, limit: int, timeout: float = 10.0) -> list[QueryAnswer]:
        """
        Samples a population of QueryAnswer objects using a remote iterator.
        """
        result = []
        start_time = time.time()
        try:
            remote_iterator = self.query_agent.pattern_matcher_query(
                tokens=self.query_tokens,
                context=self.context
            )

            while (len(result) < limit and not remote_iterator.finished()):
                if time.time() - start_time > timeout:
                    sys.stdout.write("\nTimeout reached while sampling the population.\n")
                    sys.stdout.flush()
                    break

                if (qa := remote_iterator.pop()):
                    result.append(qa)
                else:
                    time.sleep(0.1)

            time.sleep(0.5)
            return result
        except Exception as e:
            print(f"Population sampling failed: {e}")

    @log_function_call
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

    @log_function_call
    def _select_best_individuals(
        self, evaluated_individuals: list[tuple[QueryAnswer, float]]
    ) -> list[QueryAnswer]:
        """
        Selects the best individuals from evaluated QueryAnswer tuples using the configured selection method.
        """
        method = SelectionMethodType(self.selection_method)
        selection_method = handle_selection_method(method)
        return selection_method(evaluated_individuals, self.qtd_selected_for_attention_update)

    @log_function_call
    def _select_best_query_answers(self, timeout: float = 10.0) -> None:
        """
        Fills the best_query_answers queue by selecting QueryAnswers from a remote iterator.
        """
        best_answers = self._sample_population(self.max_query_answers, timeout=10.0)
        for qa in best_answers:
            with contextlib.suppress(Full):
                self.best_query_answers.put(qa, timeout=1)
