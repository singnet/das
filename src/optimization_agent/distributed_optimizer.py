import json
import time
from collections import defaultdict

from das_node.star_node import StarNode
from das_node.das_node import DASNode
from das_node.query_answer import QueryAnswer
from hyperon_das_node import Message, MessageBrokerType

from hyperon_das.cache.attention_broker_gateway import AttentionBrokerGateway

from fitness_functions import handle_fitness_function
from hyperon_das_atomdb.adapters import RedisMongoDB
from selection_methods import handle_selection_method, SelectionMethodType
from utils import Parameters, parse_file, SuppressCppOutput

DEBUG = False
SERVERS_ON_THE_NETWORK = 5
MAX_CORRELATIONS_WITHOUT_STIMULATE = 1000


# === Messages

class StartGenerationMessage(Message):
    def __init__(self, message: str):
        super().__init__()
        self.message = message

    def act(self, node: "StarNode") -> None:
        node.start(self.message)


class EvaluatedIndividualsMessage(Message):
    def __init__(self, worker_id: str, individuals: str):
        super().__init__()
        self.worker_id = worker_id
        self.individuals = individuals

    def act(self, node: "StarNode") -> None:
        node._update_remote_population(self.worker_id, self.individuals)


class WorkerStatusMessage(Message):
    def __init__(self, worker_id: str, generation: str, status: str):
        super().__init__()
        self.worker_id = worker_id
        self.generation = generation
        self.status = status

    def act(self, node: "StarNode") -> None:
        node._update_worker_status(self.worker_id, self.generation, self.status)


class LockRequestMessage(Message):
    def __init__(self, worker_id: str):
        super().__init__()
        self.worker_id = worker_id

    def act(self, node: "StarNode") -> None:
        if DEBUG:
            print(f"[LockRequestMessage.act] - The Leader will process the lock request from {self.worker_id}")
        node._process_lock_request(self.worker_id)


class LockReleaseMessage(Message):
    def __init__(self, worker_id: str):
        super().__init__()
        self.worker_id = worker_id

    def act(self, node: "StarNode") -> None:
        if DEBUG:
            print(f"[LockReleaseMessage.act] - The Leader will process the release request from {self.worker_id}")
        node._process_lock_release(self.worker_id)


class LockGrantedMessage(Message):
    def __init__(self, worker_id: str):
        super().__init__()
        self.worker_id = worker_id

    def act(self, node: "StarNode") -> None:
        if DEBUG:
            print(f"[LockGrantedMessage.act] - The worker {self.worker_id} receives authorization to execute")
        node._process_lock_granted(self.worker_id)


# === Nodes

class BaseNode(StarNode):
    def __init__(
        self,
        node_id,
        server_id: str = None,
        config_file: str = None,
        messaging_backend: MessageBrokerType = MessageBrokerType.GRPC
    ) -> None:
        super().__init__(node_id, server_id, messaging_backend)
        self.params = self.load_config(config_file)
        self.atom_db = self.connect_atom_db()
        self.known_commands = {
            "START_GENERATION": StartGenerationMessage,
            "EVALUATED_INDIVIDUALS": EvaluatedIndividualsMessage,
            "WORKER_STATUS": WorkerStatusMessage,
            "LOCK_REQUEST": LockRequestMessage,
            "LOCK_RELEASE": LockReleaseMessage,
            "LOCK_GRANTED": LockGrantedMessage
        }

    def message_factory(self, command: str, args: list[str]) -> Message:
        """
        Creates a message object based on a command and arguments.

        Args:
            command (str): The command to execute.
            args (list[str]): Arguments to pass to the command.

        Returns:
            Message: The constructed message object.
        """
        message = super().message_factory(command, args)
        if message is not None:
            return message
        if message_class := self.known_commands.get(command):
            return message_class(*args)

    def load_config(self, config_file: str) -> Parameters:
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

    def connect_atom_db(self) -> RedisMongoDB:
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


class LeaderNode(BaseNode):
    def __init__(self, node_id: str, attention_broker_server_id: str, query_tokens: str, config_file: str) -> None:
        super().__init__(node_id=node_id, config_file=config_file)
        self.attention_broker_server_id = attention_broker_server_id
        self.query_tokens = query_tokens
        self.generation = 0
        self.workers = {}
        self.joinin_requests_queue = []
        self.remote_population = {}
        self.evaluated_query = False
        # Centralized lock status
        self.critical_lock_holder = None  # Stores the ID of the worker who owns the lock
        self.lock_queue = []  # Line of workers waiting for the lock

    def optimize_query(self) -> None:
        if DEBUG:
            print('[Leader.optimize_query] - START')
        while self.generation < self.params.max_generations:
            self._start_generation(self.query_tokens)
            self._wait_for_workers()
            individuals = self._select_best_individuals()
            self._update_attention_broker(individuals)
        if DEBUG:
            print('[Leader.optimize_query] - END')
        self.evaluated_query = True

    def get_best_individuals(self, query_agent_node_id: str, query_agent_server_id: str) -> list:
        """
        Returns the best QueryAnswers after running all generations.
        """
        if not self.evaluated_query:
            return None
        answer = []
        with SuppressCppOutput():
            query_agent = DASNode(query_agent_node_id, query_agent_server_id)
            remote_iterator = query_agent.pattern_matcher_query(self.query_tokens.split(' '))
        while (len(answer) < self.params.max_query_answers and not remote_iterator.finished()):
            if (qa := remote_iterator.pop()):
                answer.append(qa)
            else:
                time.sleep(0.1)
        return answer

    def node_joined_network(self, node_id: str):
        if self.is_server:
            self.joinin_requests_queue.append(node_id)
            # print(f"[Worker] Node {node_id} wants to join. Added to JOININ_REQUESTS_QUEUE.")

            # Automatically accepts workers (tests)
            # Search how to know the number of peers in the network
            self._accept_join_request(node_id)

    def _accept_join_request(self, node_id: str):
        if node_id in self.joinin_requests_queue:
            self.joinin_requests_queue.remove(node_id)

        self.add_peer(node_id)
        self.workers[node_id] = {'generation': 1, 'status': 'live'}
        # print(f"[Worker] Node {node_id} has been accepted and added as a peer.")

    def _start_generation(self, query: str, generation_key: str = None) -> None:
        self.generation += 1
        args = query
        print(f'[Leader.start_generation] - generation: {self.generation}')
        self.broadcast('START_GENERATION', [args])

    def _wait_for_workers(self) -> None:
        if DEBUG:
            print(f'[Leader.wait_for_workers] - generation: {self.generation}')
        while not all([
            worker_data['generation'] == self.generation and worker_data['status'] == 'finished'
            for worker_data in self.workers.values()
        ]):
            time.sleep(0.1)

    def _select_best_individuals(self) -> list:
        """
        Aggregates all individuals from all workers.
        """
        if DEBUG:
            print(f'[Leader.select_best_individuals] - generation: {self.generation}')
        method = SelectionMethodType(self.params.selection_method)
        selection_method = handle_selection_method(method)

        while not self.remote_population:
            if DEBUG:
                print('No data yet')
            time.sleep(0.1)

        population = []
        for _, results in self.remote_population.items():
            for result in results:
                try:
                    population.index(result)
                except ValueError:
                    population.append(result)

        return selection_method(population, self.params.qtd_selected_for_attention_update)

    def _update_attention_broker(self, individuals: list) -> None:
        """
        Updates the attention broker by correlating handles extracted from selected QueryAnswers.

        Handles are recursively collected from each QueryAnswer, and after processing
        a defined number of correlations, the attention broker is stimulated with aggregated data.
        """
        if DEBUG:
            print(f'[Leader.update_attention_broker] - generation: {self.generation}')
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
            # handle_list.context = self.params.context
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

    def _process_individual(self, query_answer_hanldes: list[str]):
        execution_stack = query_answer_hanldes
        single_answer = set()
        joint_answer = {}
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

    def _update_worker_status(self, worker_id: str, generation: str, status: str):
        self.workers[worker_id] = {
            'generation': int(generation),
            'status': status
        }

    def _update_remote_population(self, worker_id: str, individuals: str):
        self.remote_population[worker_id] = json.loads(individuals)

    def _process_lock_request(self, worker_id: str):
        if self.critical_lock_holder is None:
            self.critical_lock_holder = worker_id
            # Sends a message to the worker informing him that he can execute
            self.send("LOCK_GRANTED", [worker_id], worker_id)
        else:
            if worker_id not in self.lock_queue:
                self.lock_queue.append(worker_id)

    def _process_lock_release(self, worker_id: str):
        if self.critical_lock_holder == worker_id:
            self.critical_lock_holder = None
            if self.lock_queue:
                next_worker = self.lock_queue.pop(0)
                self.critical_lock_holder = next_worker
                self.send("LOCK_GRANTED", [next_worker], next_worker)


class WorkerNode(BaseNode):
    def __init__(self, node_id: str, server_id: str, query_agent_node_id: str, query_agent_server_id: str, config_file: str):
        super().__init__(node_id=node_id, server_id=server_id, config_file=config_file)
        self.node_id = node_id
        self.query_agent = DASNode(query_agent_node_id, query_agent_server_id)
        self.generation = 0
        self.atom_db = self.connect_atom_db()
        # Flag to signal that the worker has received the lock
        self.lock_acquired = False

    def start(self, message):
        if DEBUG:
            print(f"[Worker.start] - {self.node_id}")
        self.generation += 1

        self._send_status("running")

        # Asks Leader for access to Lock
        self.send("LOCK_REQUEST", [self.node_id], self.server_id)

        # Worker must wait for the lock_granted message.
        while not self.lock_acquired:
            time.sleep(0.1)

        self._execute(message)

        # Release the lock
        self.send("LOCK_RELEASE", [self.node_id], self.server_id)
        self.lock_acquired = False

    def _execute(self, message: str) -> None:
        try:
            population = self._sample_population(message)

            if not population:
                self._send_status("fail:sample_population")
                time.sleep(1)
                if DEBUG:
                    print(f"[Worker.sample_population] Try again - {self.node_id}")
                population = self._sample_population(message)

            if DEBUG:
                print(f"[Worker.evaluate] - {self.node_id}")
            results = []
            for indiv in population:
                fit_val = self._evaluate(indiv)
                results.append((indiv.get_handles(), fit_val))

            self._send_evaluated_individuals(results)
        except Exception as e:
            self._send_status(f"fail:{e}")

    def _sample_population(self, query_tokens: str) -> list[QueryAnswer]:
        """
        Samples a population of QueryAnswer objects using a remote iterator.
        """
        if DEBUG:
            print(f"[Worker.sample_population] - {self.node_id}")
        result = []
        try:
            with SuppressCppOutput():
                remote_iterator = self.query_agent.pattern_matcher_query(query_tokens.split(' '))

            while (len(result) < self.params.population_size and not remote_iterator.finished()):
                if (qa := remote_iterator.pop()):
                    result.append(qa)
                else:
                    time.sleep(0.1)

            time.sleep(0.5)
        except Exception as e:
            print(f"Population sampling failed: {e}")
        return result

    def _evaluate(self, query_answer: QueryAnswer) -> float:
        """
        Evaluates a QueryAnswer using a configured fitness function.
        """
        fitness_function = handle_fitness_function(self.params.fitness_function)
        return fitness_function(self.atom_db, query_answer)

    def _process_lock_granted(self, worker_id: str) -> None:
        if self.node_id == worker_id:
            self.lock_acquired = True

    def _send_evaluated_individuals(self, individuals: list) -> None:
        if DEBUG:
            print(f"[Worker._send_evaluated_individuals] - {self.node_id}")
        self.send('EVALUATED_INDIVIDUALS', [self.node_id, json.dumps(individuals)], self.server_id)
        self._send_status("finished")

    def _send_status(self, status: str) -> None:
        self.send("WORKER_STATUS", [self.node_id, str(self.generation), status], self.server_id)


if __name__ == '__main__':
    query = 'LINK_TEMPLATE Evaluation 2 NODE Type Name VARIABLE V1'
    path = "/home/marcocapozzoli/Desktop/hub-potencializa/jobs/singularity-net/projects/das/src/optimization_agent/distributed_optimizer_config.cfg"

    leader = LeaderNode("localhost:7000", "localhost:37007", query, path)

    worker1 = WorkerNode("localhost:7001", "localhost:7000", "localhost:31701", "localhost:31700", path)
    worker2 = WorkerNode("localhost:7002", "localhost:7000", "localhost:31702", "localhost:31700", path)
    # worker3 = WorkerNode("localhost:7003", "localhost:7000", "localhost:31703", "localhost:31700", path)
    # worker4 = WorkerNode("localhost:7004", "localhost:7000", "localhost:31704", "localhost:31700", path)
    # worker5 = WorkerNode("localhost:7005", "localhost:7000", "localhost:31705", "localhost:31700", path)

    print("\n== START ==\n")

    leader.optimize_query()
    answers = leader.get_best_individuals("localhost:31706", "localhost:31700")

    print("\n== END ==\n")
