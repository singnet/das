import pytest
import random
import grpc
from unittest import mock

from evolution.optimizer import QueryOptimizerAgent, QueryOptimizerIterator
from evolution.fitness_functions import multiply_strengths, FitnessFunctions
from hyperon_das_atomdb.exceptions import AtomDoesNotExist
from evolution.selection_methods import SelectionMethodType, handle_selection_method, roulette, best_fitness
from evolution.node import NodeIdFactory, EvolutionNode, EvolutionRequest
from evolution.attention_broker_updater import AttentionBrokerUpdater
from evolution.utils import Parameters, SuppressCppOutput
from hyperon_das_atomdb.adapters import RedisMongoDB
from concurrent.futures import ThreadPoolExecutor


# NOTE: WIP
# class TestQueryOptimizer:
#     class DummyQueryAnswer:
#         def __init__(self, text="dummy"):
#             self.text = text
#             self.handles = []

#         def to_string(self):
#             return self.text

#     class DummyRemoteIterator:
#         def __init__(self, answers):
#             self.answers = answers
#             self.index = 0

#         def finished(self):
#             return self.index >= len(self.answers)

#         def pop(self):
#             if self.index < len(self.answers):
#                 ans = self.answers[self.index]
#                 self.index += 1
#                 return ans
#             return None

#     class DummyDASNode:
#         def __init__(self, node_id, server_id):
#             self.node_id = node_id
#             self.server_id = server_id

#         def pattern_matcher_query(self, query_tokens):
#             return TestQueryOptimizer.DummyRemoteIterator([TestQueryOptimizer.DummyQueryAnswer("answer1"), TestQueryOptimizer.DummyQueryAnswer("answer2")])

#     class DummyRedisMongoDB:
#         def __init__(self, **kwargs):
#             pass

#         def get_link_targets(self, handle):
#             if handle == "a":
#                 return ["b"]
#             return []

#     class DummyAttentionBrokerGateway:
#         def __init__(self, config):
#             self.config = config
#             self.last_stimulate = None  # to capture the parameters used in stimulate

#         def correlate(self, single_answer):
#             pass

#         def stimulate(self, handle_count):
#             self.last_stimulate = handle_count

#     def dummy_fitness_function_handle(name):
#         def fitness(db, handles):
#             return 0.5

#         return fitness

#     def write_config_file(tmp_path, config_data):
#         config_file = tmp_path / "config.cfg"
#         lines = []
#         for key, value in config_data.items():
#             lines.append(f"{key}={value}")
#         config_file.write_text("\n".join(lines))
#         return str(config_file)

#     def get_minimal_config():
#         return {
#             "max_query_answers": 2,
#             "max_generations": 1,
#             "population_size": 2,
#             "qtd_selected_for_attention_update": 1,
#             "selection_method": "roulette",
#             "fitness_function": "multiply_strengths",
#             "attention_broker_server_id": "localhost:1234",
#             "query_agent_node_id": "localhost:2000",
#             "query_agent_server_id": "localhost:2001",
#             "mongo_hostname": "localhost",
#             "mongo_port": 27017,
#             "mongo_username": "user",
#             "mongo_password": "pass",
#             "redis_hostname": "localhost",
#             "redis_port": 6379,
#             "redis_cluster": False,
#             "redis_ssl": False,
#         }

#     @pytest.fixture
#     def valid_config_path(self, tmp_path):
#         config_data = TestQueryOptimizer.get_minimal_config()
#         return TestQueryOptimizer.write_config_file(tmp_path, config_data)

#     @pytest.fixture
#     def invalid_config_path(self, tmp_path):
#         config_data = TestQueryOptimizer.get_minimal_config()
#         config_data["qtd_selected_for_attention_update"] = 4
#         return TestQueryOptimizer.write_config_file(tmp_path, config_data)

#     @pytest.fixture
#     def optimizer(self):
#         with (
#             mock.patch("evolution.optimizer.DASNode", new=TestQueryOptimizer.DummyDASNode),
#             mock.patch("evolution.optimizer.RedisMongoDB", new=TestQueryOptimizer.DummyRedisMongoDB),
#             mock.patch(
#                 "evolution.optimizer.FitnessFunctions.get", new=TestQueryOptimizer.dummy_fitness_function_handle
#             ),
#             mock.patch(
#                 "evolution.optimizer.AttentionBrokerGateway", new=TestQueryOptimizer.DummyAttentionBrokerGateway
#             ),
#             mock.patch.object(QueryOptimizerIterator, "_producer", lambda self: None),
#         ):
#             return QueryOptimizerIterator(query_tokens=["token"], **TestQueryOptimizer.get_minimal_config())

#     def test_query_optimizer_agent_success(self, valid_config_path):
#         with (
#             mock.patch("evolution.optimizer.DASNode", new=TestQueryOptimizer.DummyDASNode),
#             mock.patch("evolution.optimizer.RedisMongoDB", new=TestQueryOptimizer.DummyRedisMongoDB),
#             mock.patch(
#                 "evolution.optimizer.FitnessFunctions.get", new=TestQueryOptimizer.dummy_fitness_function_handle
#             ),
#             mock.patch(
#                 "evolution.optimizer.AttentionBrokerGateway", new=TestQueryOptimizer.DummyAttentionBrokerGateway
#             ),
#         ):
#             agent = QueryOptimizerAgent(valid_config_path)
#             iterator = agent.optimize("token1 token2")

#             assert isinstance(iterator, QueryOptimizerIterator)
#             assert iterator.is_empty() is False
#             assert iterator.query_tokens == ["token1", "token2"]

#             answers = list(iterator)

#             assert iterator.is_empty() is True
#             assert len(answers) == 2
#             assert sorted([answer.to_string() for answer in answers]) == sorted(
#                 ["answer1", "answer2"]
#             )

#     def test_query_optimizer_agent_invalid_config(self, invalid_config_path):
#         with pytest.raises(ValueError):
#             QueryOptimizerAgent(invalid_config_path)

#     def test_evaluate_method(self):
#         with (
#             mock.patch("evolution.optimizer.DASNode", new=DummyDASNode),
#             mock.patch("evolution.optimizer.RedisMongoDB", new=DummyRedisMongoDB),
#             mock.patch(
#                 "evolution.optimizer.FitnessFunctions.get", new=dummy_fitness_function_handle
#             ),
#             mock.patch(
#                 "evolution.optimizer.AttentionBrokerGateway", new=DummyAttentionBrokerGateway
#             ),
#             mock.patch.object(QueryOptimizerIterator, "_producer", lambda self: None),
#         ):
#             optimizer = QueryOptimizerIterator(query_tokens=["token"], **get_minimal_config())
#             dqa = DummyQueryAnswer("test")
#             result = optimizer._evaluate(dqa)

#             assert isinstance(result, tuple)
#             assert result[0] == dqa
#             assert result[1] == 0.5

#     def test_process_individual(self, optimizer):
#         dqa = DummyQueryAnswer("test")
#         dqa.handles = ["a"]

#         single_answer, joint_answer = optimizer._process_individual(dqa)

#         assert single_answer == {"a", "b"}
#         assert joint_answer == {"a": 1, "b": 1}

#     def test_attention_broker_stimulate(self, optimizer):
#         dummy_broker = DummyAttentionBrokerGateway(
#             {"attention_broker_hostname": "localhost", "attention_broker_port": "1234"}
#         )

#         joint_answer = {"a": 2, "b": 3}

#         optimizer._attention_broker_stimulate(dummy_broker, joint_answer)

#         assert dummy_broker.last_stimulate is not None

#         expected = {"a": 2, "b": 3, "SUM": 5}

#         assert dummy_broker.last_stimulate == expected

#     def test_select_best_individuals(self, optimizer):
#         ind1 = DummyQueryAnswer("A")
#         ind2 = DummyQueryAnswer("B")
#         ind3 = DummyQueryAnswer("C")
#         ind4 = DummyQueryAnswer("D")
#         ind5 = DummyQueryAnswer("E")

#         evaluated = [(ind1, 0.63), (ind2, 0.91), (ind3, 0.57), (ind4, 0.35), (ind5, 0.90)]

#         original_random = random.random
#         try:
#             random.random = lambda: 0.2  # set random.random() to 0.2
#             selected = optimizer._select_best_individuals(evaluated)
#         finally:
#             random.random = original_random

#         assert selected == [ind2]

#     def test_iterator_stop_iteration(self, optimizer):
#         optimizer.producer_finished.set()
#         with pytest.raises(StopIteration):
#             next(optimizer)

#     def test_terator_is_empty(self, optimizer):
#         optimizer.producer_finished.set()
#         assert optimizer.is_empty()


class TestFitnessFunctions:
    class DummyAtom:
        def __init__(self, custom_attributes=None):
            self.custom_attributes = custom_attributes or {}

    class DummyAtomDB:
        def __init__(self, atoms: dict):
            self.atoms = atoms

        def get_atom(self, handle):
            if handle in self.atoms:
                return self.atoms[handle]
            else:
                raise AtomDoesNotExist(f"Atom {handle} does not exist")

    def test_multiply_strengths_all_valid(self):
        atoms = {
            "atom1": self.DummyAtom({"strength": 0.22}),
            "atom2": self.DummyAtom({"strength": 0.23}),
            "atom3": self.DummyAtom({"strength": 0.37}),
        }
        atom_db = self.DummyAtomDB(atoms)
        handles = ["atom1", "atom2", "atom3"]
        result = multiply_strengths(atom_db, handles)

        assert result == 0.018722

    def test_multiply_strengths_missing_atom(self, capsys):
        atoms = {
            "atom1": self.DummyAtom({"strength": 0.82}),
        }
        atom_db = self.DummyAtomDB(atoms)
        handles = ["atom1", "atom_missing"]
        result = multiply_strengths(atom_db, handles)

        assert result == 0.82
        captured = capsys.readouterr().out
        assert "Error:" in captured

    def test_multiply_strengths_no_strength_attribute(self):
        atoms = {
            "atom1": self.DummyAtom({"confidence": 0.25}),
            "atom2": self.DummyAtom(),
        }
        atom_db = self.DummyAtomDB(atoms)
        handles = ["atom1", "atom2"]
        result = multiply_strengths(atom_db, handles)

        assert result == 0.0

    def test_multiply_strengths_mixed(self, capsys):
        atoms = {
            "atom1": self.DummyAtom({"strength": 0.012}),
            "atom2": self.DummyAtom({"strength": None}),
            "atom3": self.DummyAtom({"confidence": 0.55}),
        }
        atom_db = self.DummyAtomDB(atoms)
        handles = ["atom1", "atom2", "atom3", "atom_missing"]
        result = multiply_strengths(atom_db, handles)

        assert result == 0.012
        captured = capsys.readouterr().out
        assert "Error:" in captured

    def test_multiply_strengths_empty_handles(self):
        atom_db = self.DummyAtomDB({})
        result = multiply_strengths(atom_db, [])
        assert result == 0.0

    def test_fitness_functions_get_valid(self):
        func = FitnessFunctions.get("multiply_strengths")
        assert callable(func)
        assert func == multiply_strengths

    def test_fitness_functions_get_invalid(self):
        with pytest.raises(ValueError) as excinfo:
            FitnessFunctions.get("non_existing_function")
        assert "Unknown fitness function" in str(excinfo.value)


class TestSelectionMethods:
    def test_handle_selection_method(self):
        func = handle_selection_method(SelectionMethodType.BEST_FITNESS)
        assert func == best_fitness

        func = handle_selection_method(SelectionMethodType.ROULETTE)
        assert func == roulette

        with pytest.raises(ValueError) as excinfo:
            handle_selection_method("invalid_method")
        assert "Unknown selection method type" in str(excinfo.value)

    def test_best_fitness_selection(self):
        population = [
            ("ind1", 0.1),
            ("ind2", 0.2),
            ("ind3", 0.15),
            ("ind4", 0.5)
        ]
        selected = best_fitness(population, 2)
        assert selected == [("ind4", 0.5), ("ind2", 0.2)]

    def test_best_fitness_selection_less_than_max(self):
        population = [
            ("ind1", 0.1),
            ("ind2", 0.2)
        ]
        selected = best_fitness(population, 5)
        assert selected == [("ind2", 0.2), ("ind1", 0.1)]

    def test_best_fitness_empty_population(self):
        selected = best_fitness([], 3)
        assert selected == []

    def test_roulette_selection(self):
        population = [
            ("ind1", 0.11),
            ("ind2", 0.33),
            ("ind3", 0.22)
        ]
        selected = roulette(population, 2)
        assert len(selected) == 2
        assert all(ind in ["ind1", "ind2", "ind3"] for ind, _ in selected)

    def test_roulette_total_fitness_zero(self, capsys):
        population = [
            ("ind1", 0.0),
            ("ind2", 0.0)
        ]
        selected = roulette(population, 2)
        assert selected == []
        captured = capsys.readouterr().out
        assert "Total fitness is zero" in captured

    def test_roulette_empty_population(self, capsys):
        selected = roulette([], 3)
        assert selected == []
        captured = capsys.readouterr().out
        assert "Total fitness is zero" in captured

    def test_roulette_selection_less_than_max(self):
        population = [
            ("ind1", 0.81),
            ("ind2", 0.43)
        ]
        selected = roulette(population, 5)
        assert len(selected) == 2
        assert all(ind in ["ind1", "ind2"] for ind, _ in selected)


class TestEvolutionNode:
    def test_node_id_factory_cycle(self):
        factory = NodeIdFactory(ip="127.0.0.1", start_port=100, end_port=101)
        node_id1 = factory.generate_node_id()
        node_id2 = factory.generate_node_id()
        node_id3 = factory.generate_node_id()
        assert node_id1 == "127.0.0.1:100"
        assert node_id2 == "127.0.0.1:101"
        assert node_id3 == "127.0.0.1:100"

    def test_evolution_request_act(self):
        node = EvolutionNode("localhost:0000")
        req = EvolutionRequest("localhost:1234,localhost:4321", "test_context", "arg1", "arg2")
        req.act(node)
        expected = {'senders': ["localhost:1234", "localhost:4321"], 'context': "test_context", 'data': ("arg1", "arg2")}
        assert node.request_queue.qsize() == 1
        assert node.request_queue.empty() is False
        assert node.request_queue.get() == expected

    def test_evolution_node_init(self):
        factory = NodeIdFactory(ip="localhost")
        node = EvolutionNode(server_id="localhost:0000", node_id_factory=factory)
        assert node.node_id() == "localhost:20000"

        with pytest.raises(ValueError) as excinfo:
            EvolutionNode(node_id=None, server_id="localhost:0000", node_id_factory=None)
        assert "For clients, a node_id factory must be provided" in str(excinfo.value)

    def test_message_factory_with_super_message(self):
        original_message_factory = EvolutionNode.__bases__[0].message_factory
        EvolutionNode.__bases__[0].message_factory = lambda self, command, args: "dummy_super_message"
        node = EvolutionNode(node_id="localhost:8081", server_id="localhost:8080")
        result = node.message_factory("any_command", ["arg"])
        assert result == "dummy_super_message"
        EvolutionNode.__bases__[0].message_factory = original_message_factory

    def test_message_factory_with_known_command(self):
        original_message_factory = EvolutionNode.__bases__[0].message_factory
        EvolutionNode.__bases__[0].message_factory = lambda self, command, args: None
        node = EvolutionNode(node_id="localhost:8081", server_id="localhost:8080")
        result = node.message_factory("evolution_request", ["localhost:1234,localhost:4321", "test_context", "arg1"])
        assert isinstance(result, EvolutionRequest)
        assert result.senders == ["localhost:1234", "localhost:4321"]
        assert result.context == "test_context"
        assert result.request == ("arg1",)
        EvolutionNode.__bases__[0].message_factory = original_message_factory

    def test_add_and_pop_request_as_leader(self):
        node = EvolutionNode(node_id="localhost:8080")
        assert node.request_queue.empty()
        node.add_request(["localhost:1234", "localhost:4321"], "context_test", "data_test")
        assert node.request_queue.empty() is False
        request = node.pop_request()
        expected = {'senders': ["localhost:1234", "localhost:4321"], 'context': "context_test", 'data': "data_test"}
        assert request == expected
        assert node.request_queue.empty()

    def test_add_and_pop_request_as_non_leader(self):
        node = EvolutionNode(node_id="localhost:8081", server_id="localhost:8080", node_id_factory=NodeIdFactory("localhost"))
        assert node.is_leader() is False
        node.add_request(["sender"], "context", "data")
        assert node.request_queue.empty()
        node.request_queue.put({'senders': ["sender"], 'context': "context", 'data': "data"})
        result = node.pop_request()
        assert result is None


class TestAttentionBrokerUpdater:
    class DummyAck:
        def __init__(self, msg):
            self.msg = msg

    class DummyChannel:
        def __init__(self, response_msg):
            self.response_msg = response_msg
            self.last_message = None

        def unary_unary(self, path, request_serializer, response_deserializer):
            def func(message):
                self.last_message = message
                return TestAttentionBrokerUpdater.DummyAck(self.response_msg)
            return func

        def __enter__(self):
            return self

        def __exit__(self, exc_type, exc_val, exc_tb):
            pass

    class DummyAtomDB:
        def __init__(self, target_map=None, raise_for=None):
            self.target_map = target_map or {}
            self.raise_for = raise_for or []
            self.called_handles = []

        def get_link_targets(self, handle):
            self.called_handles.append(handle)
            if handle in self.raise_for:
                raise Exception("Simulated exception")
            return self.target_map.get(handle, [])

    class DummyQueryAnswer:
        def __init__(self, handles):
            self.handles = handles
            self.handles_size = len(handles)

    class DummyHandleList:
        def __init__(self, list, context):
            self.list = list
            self.context = context

        def SerializeToString(self):
            return b"dummy_handle_list"

    class DummyHandleCount:
        def __init__(self, map, context):
            self.map = map
            self.context = context

        def SerializeToString(self):
            return b"dummy_handle_count"

    def test_update_normal(self, monkeypatch):
        query = TestAttentionBrokerUpdater.DummyQueryAnswer(["A", "B"])
        individuals = [(query, 0.5)]
        atomdb = TestAttentionBrokerUpdater.DummyAtomDB()
        channels = [
            TestAttentionBrokerUpdater.DummyChannel("CORRELATE"),
            TestAttentionBrokerUpdater.DummyChannel("STIMULATE")
        ]

        def fake_insecure_channel(address):
            return channels.pop(0)

        monkeypatch.setattr(grpc, "insecure_channel", fake_insecure_channel)
        updater = AttentionBrokerUpdater("dummy_address", atomdb, context="normal")
        updater.update(individuals)
        assert set(atomdb.called_handles) == {"A", "B"}
        assert len(channels) == 0

    def test_update_get_link_targets_exception(self, monkeypatch, capsys):
        query = TestAttentionBrokerUpdater.DummyQueryAnswer(["X"])
        individuals = [(query, 0.5)]
        atomdb = TestAttentionBrokerUpdater.DummyAtomDB(raise_for=["X"])
        channels = [
            TestAttentionBrokerUpdater.DummyChannel("CORRELATE"),
            TestAttentionBrokerUpdater.DummyChannel("STIMULATE")
        ]

        def fake_insecure_channel(address):
            return channels.pop(0)

        monkeypatch.setattr(grpc, "insecure_channel", fake_insecure_channel)
        updater = AttentionBrokerUpdater("dummy_address", atomdb, context="exception")
        updater.update(individuals)
        captured = capsys.readouterr().out

        # The simulated exception is caught internally and should not be printed
        assert "Simulated exception" not in captured

    def test_update_correlate_failure(self, monkeypatch, capsys):
        query = TestAttentionBrokerUpdater.DummyQueryAnswer(["A"])
        individuals = [(query, 0.5)]
        atomdb = TestAttentionBrokerUpdater.DummyAtomDB()

        channels = [
            TestAttentionBrokerUpdater.DummyChannel("FAIL"),
            TestAttentionBrokerUpdater.DummyChannel("STIMULATE")
        ]
        
        def fake_insecure_channel(address):
            return channels.pop(0)
        
        monkeypatch.setattr(grpc, "insecure_channel", fake_insecure_channel)
        updater = AttentionBrokerUpdater("dummy_address", atomdb, context="correlate_fail")
        updater.update(individuals)
        captured = capsys.readouterr().out
        assert "Failed GRPC command: AttentionBroker.correlate()" in captured

    def test_update_stimulate_failure(self, monkeypatch, capsys):
        query = TestAttentionBrokerUpdater.DummyQueryAnswer(["A"])
        individuals = [(query, 0.5)]
        atomdb = TestAttentionBrokerUpdater.DummyAtomDB()

        channels = [
            TestAttentionBrokerUpdater.DummyChannel("CORRELATE"),
            TestAttentionBrokerUpdater.DummyChannel("FAIL")
        ]

        def fake_insecure_channel(address):
            return channels.pop(0)
        
        monkeypatch.setattr(grpc, "insecure_channel", fake_insecure_channel)
        updater = AttentionBrokerUpdater("dummy_address", atomdb, context="stimulate_fail")
        updater.update(individuals)
        captured = capsys.readouterr().out
        assert "Failed GRPC command: AttentionBroker.stimulate()" in captured

    def test_update_execution_stack_processing(self, monkeypatch):
        query = TestAttentionBrokerUpdater.DummyQueryAnswer(["A"])
        individuals = [(query, 0.5)]
        atomdb = TestAttentionBrokerUpdater.DummyAtomDB(target_map={"A": ["B"], "B": []})
        channels = [
            TestAttentionBrokerUpdater.DummyChannel("CORRELATE"),
            TestAttentionBrokerUpdater.DummyChannel("STIMULATE")
        ]

        def fake_insecure_channel(address):
            return channels.pop(0)

        monkeypatch.setattr(grpc, "insecure_channel", fake_insecure_channel)
        updater = AttentionBrokerUpdater("dummy_address", atomdb, context="execution")
        updater.update(individuals)
        assert set(atomdb.called_handles) >= {"A", "B"}

    def test_update_empty_individuals(self, monkeypatch):
        individuals = []
        atomdb = TestAttentionBrokerUpdater.DummyAtomDB()

        def fake_insecure_channel(address):
            raise Exception("insecure_channel should not be called")

        monkeypatch.setattr(grpc, "insecure_channel", fake_insecure_channel)
        updater = AttentionBrokerUpdater("dummy_address", atomdb, context="empty")
        updater.update(individuals)

    def test_update_max_correlations(self, monkeypatch):
        individuals = [
            (TestAttentionBrokerUpdater.DummyQueryAnswer([str(i)]), 0.5)
            for i in range(1001)
        ]
        atomdb = TestAttentionBrokerUpdater.DummyAtomDB()
        channels = []

        for _ in range(1001):
            channels.append(TestAttentionBrokerUpdater.DummyChannel("CORRELATE"))
        channels.append(TestAttentionBrokerUpdater.DummyChannel("STIMULATE"))

        def fake_insecure_channel(address):
            return channels.pop(0)

        monkeypatch.setattr(grpc, "insecure_channel", fake_insecure_channel)
        updater = AttentionBrokerUpdater("dummy_address", atomdb, context="max_correlations")
        updater.update(individuals)

        assert len([c for c in channels if c.response_msg == "STIMULATE"]) == 1
