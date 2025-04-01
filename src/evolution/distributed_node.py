from enum import Enum, auto
from typing import Any
from hyperon_das_node import StarNode, QueryAnswer

from src.evolution.utils import Parameters, parse_file


class State(Enum):
    INITIAL = auto()
    SAMPLING = auto()
    WAITING = auto()
    NETWORK_UPDATE = auto()
    FINISHED = auto()


class DistributedEvolutionNode(StarNode):
    def __init__(self, node_id: str, server_id: str = None, config_file: str = None) -> None:
        self.state = State.INITIAL
        self.population = []
        self.remote_population = {}
        self.joinin_requests_queue = set()
        self.params = None
        if server_id is None:
            if not config_file:
                raise ValueError("For server-type nodes, a configuration file ('config_file') must be provided.")
            self.params = self._load_config(config_file)
            self.current_generation = 1
            super().__init__(node_id)
        else:
            super().__init__(node_id, server_id)

    def transition(self, new_state: State) -> None:
        self.state = new_state

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

    def node_joined_network(self, node_id: str) -> None:
        if self.is_leader():
            self.joinin_requests_queue.add(node_id)

            # For now all nodes are accepted
            self._accept_join_request(node_id)

    def _accept_join_request(self, node_id: str) -> None:
        if node_id in self.joinin_requests_queue:
            self.joinin_requests_queue.remove(node_id)
        super().node_joined_network(node_id)

    def run(self) -> None:
        """
        Run the node's control state machine in a loop until the stopping criterion is met.
        """
        if self.state == State.INITIAL:
            self.initialize()
            self.transition(State.SAMPLING)

        # Main loop for processing generations.
        while not self.stop_criteria():
            if self.state == State.SAMPLING:
                self.sample_population()
                self.transition(State.WAITING)
            if self.state == State.WAITING:
                self.wait_for_others()
                self.transition(State.NETWORK_UPDATE)
            if self.state == State.NETWORK_UPDATE:
                self.network_update()
                self.transition(State.SAMPLING)
            
            if self.is_leader():
                self.current_generation += 1

    def current_state(self) -> dict[str, Any]:
        """
        Retrieve the current state of the node.

        Returns:
            dict[str, Any]: A dictionary representing the current state of the node,
                including relevant attributes such as population, remote population, and
                other state-related information.
        """

    def stop_criteria(self) -> bool:
        """
        Determine whether the stopping criterion is met.

        Returns:
            bool: True if the stopping criterion is met, False otherwise.
        """

    def initialize(self) -> None:
        """
        Start a new generation by transitioning the node's state and
        broadcasting a start signal if the node is the leader.
        """

    def sample_population(self) -> None:
        """
        Sample the local population by performing a query, computing fitness,
        and broadcasting each individual.
        """

    def perform_query(self) -> QueryAnswer:
        """
        Perform a query to sample a new QueryAnswer.

        Returns:
            QueryAnswer: A new query answer object.
        """

    def compute_fitness(self, query_answer: QueryAnswer) -> float:
        """
        Compute the fitness value for a given QueryAnswer.

        Args:
            query_answer (QueryAnswer): The query answer to evaluate.

        Returns:
            float: The computed fitness value.
        """

    def wait_for_others(self) -> None:
        """
        Wait for other nodes to finish sending their individuals.
        Process incoming messages and update the remote population data.
        """

    def check_all_nodes_received(self) -> bool:
        """
        Check if all live nodes have sent the required number of individuals.
        
        Returns:
            bool: True if all nodes have sent their individuals, False otherwise.
        """

    def spread_activation(self) -> None:
        """
        Leader node selects individuals from the combined populations and sends
        stimuli using an AttentionBroker mechanism.
        """

    def send_stimuli(self) -> None:
        """
        Send stimuli using the AttentionBroker.
        """

    def reset_populations(self) -> None:
        """
        Reset the local and remote populations to prepare for the next generation.
        """

    def network_update(self) -> None:
        """
        Update the network by processing joining requests. The leader sends
        node lists to new nodes and informs all nodes about the new node.
        """
