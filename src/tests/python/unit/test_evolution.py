import pytest
import random

from unittest import mock

from evolution.optimizer import QueryOptimizerAgent, QueryOptimizerIterator


class DummyQueryAnswer:
    def __init__(self, text="dummy"):
        self.text = text
        self.handles = []

    def to_string(self):
        return self.text


class DummyRemoteIterator:
    def __init__(self, answers):
        self.answers = answers
        self.index = 0

    def finished(self):
        return self.index >= len(self.answers)

    def pop(self):
        if self.index < len(self.answers):
            ans = self.answers[self.index]
            self.index += 1
            return ans
        return None


class DummyDASNode:
    def __init__(self, node_id, server_id):
        self.node_id = node_id
        self.server_id = server_id

    def pattern_matcher_query(self, query_tokens):
        return DummyRemoteIterator([DummyQueryAnswer("answer1"), DummyQueryAnswer("answer2")])


class DummyRedisMongoDB:
    def __init__(self, **kwargs):
        pass

    def get_link_targets(self, handle):
        if handle == "a":
            return ["b"]
        return []


class DummyAttentionBrokerGateway:
    def __init__(self, config):
        self.config = config
        self.last_stimulate = None  # to capture the parameters used in stimulate

    def correlate(self, single_answer):
        pass

    def stimulate(self, handle_count):
        self.last_stimulate = handle_count


def dummy_fitness_function_handle(name):
    def fitness(db, handles):
        return 0.5
    return fitness


def write_config_file(tmp_path, config_data):
    config_file = tmp_path / "config.cfg"
    lines = []
    for key, value in config_data.items():
        lines.append(f"{key}={value}")
    config_file.write_text("\n".join(lines))
    return str(config_file)


def get_minimal_config():
    return {
        "max_query_answers": 2,
        "max_generations": 1,
        "population_size": 2,
        "qtd_selected_for_attention_update": 1,
        "selection_method": "roulette",
        "fitness_function": "multiply_strengths",
        "attention_broker_server_id": "localhost:1234",
        "query_agent_node_id": "localhost:2000",
        "query_agent_server_id": "localhost:2001",
        "mongo_hostname": "localhost",
        "mongo_port": 27017,
        "mongo_username": "user",
        "mongo_password": "pass",
        "redis_hostname": "localhost",
        "redis_port": 6379,
        "redis_cluster": False,
        "redis_ssl": False,
    }


class TestQueryOptimizer:
    @pytest.fixture
    def valid_config_path(self, tmp_path):
        config_data = get_minimal_config()
        return write_config_file(tmp_path, config_data)

    @pytest.fixture
    def invalid_config_path(self, tmp_path):
        config_data = get_minimal_config()
        config_data["qtd_selected_for_attention_update"] = 4
        return write_config_file(tmp_path, config_data)

    @pytest.fixture
    def optimizer(self):
        with mock.patch(
            "evolution.optimizer.DASNode", new=DummyDASNode
        ), mock.patch(
            "evolution.optimizer.RedisMongoDB", new=DummyRedisMongoDB
        ), mock.patch(
            "evolution.optimizer.FitnessFunctions.get", new=dummy_fitness_function_handle
        ), mock.patch(
            "evolution.optimizer.AttentionBrokerGateway", new=DummyAttentionBrokerGateway
        ), mock.patch.object(QueryOptimizerIterator, "_producer", lambda self: None):
            return QueryOptimizerIterator(
                query_tokens=["token"],
                **get_minimal_config()
            )

    def test_query_optimizer_agent_success(self, valid_config_path):
        with mock.patch(
            "evolution.optimizer.DASNode", new=DummyDASNode
        ), mock.patch(
            "evolution.optimizer.RedisMongoDB", new=DummyRedisMongoDB
        ), mock.patch(
            "evolution.optimizer.FitnessFunctions.get", new=dummy_fitness_function_handle
        ), mock.patch("evolution.optimizer.AttentionBrokerGateway", new=DummyAttentionBrokerGateway):

            agent = QueryOptimizerAgent(valid_config_path)
            iterator = agent.optimize("token1 token2")

            assert isinstance(iterator, QueryOptimizerIterator)
            assert iterator.is_empty() is False
            assert iterator.query_tokens == ["token1", "token2"]

            answers = list(iterator)

            assert iterator.is_empty() is True
            assert len(answers) == 2
            assert sorted([answer.to_string() for answer in answers]) == sorted(["answer1", "answer2"])

    def test_query_optimizer_agent_invalid_config(self, invalid_config_path):
        with pytest.raises(ValueError):
            QueryOptimizerAgent(invalid_config_path)

    def test_evaluate_method(self):
        with mock.patch(
            "evolution.optimizer.DASNode", new=DummyDASNode
        ), mock.patch(
            "evolution.optimizer.RedisMongoDB", new=DummyRedisMongoDB
        ), mock.patch(
            "evolution.optimizer.FitnessFunctions.get", new=dummy_fitness_function_handle
        ), mock.patch(
            "evolution.optimizer.AttentionBrokerGateway", new=DummyAttentionBrokerGateway
        ), mock.patch.object(QueryOptimizerIterator, "_producer", lambda self: None):
            optimizer = QueryOptimizerIterator(
                query_tokens=["token"],
                **get_minimal_config()
            )
            dqa = DummyQueryAnswer("test")
            result = optimizer._evaluate(dqa)

            assert isinstance(result, tuple)
            assert result[0] == dqa
            assert result[1] == 0.5

    def test_process_individual(self, optimizer):
        dqa = DummyQueryAnswer("test")
        dqa.handles = ["a"]

        single_answer, joint_answer = optimizer._process_individual(dqa)

        assert single_answer == {"a", "b"}
        assert joint_answer == {"a": 1, "b": 1}

    def test_attention_broker_stimulate(self, optimizer):
        dummy_broker = DummyAttentionBrokerGateway({"attention_broker_hostname": "localhost", "attention_broker_port": "1234"})

        joint_answer = {"a": 2, "b": 3}

        optimizer._attention_broker_stimulate(dummy_broker, joint_answer)

        assert dummy_broker.last_stimulate is not None

        expected = {"a": 2, "b": 3, "SUM": 5}

        assert dummy_broker.last_stimulate == expected

    def test_select_best_individuals(self, optimizer):
        ind1 = DummyQueryAnswer("A")
        ind2 = DummyQueryAnswer("B")
        ind3 = DummyQueryAnswer("C")
        ind4 = DummyQueryAnswer("D")
        ind5 = DummyQueryAnswer("E")

        evaluated = [(ind1, 0.63), (ind2, 0.91), (ind3, 0.57), (ind4, 0.35), (ind5, 0.90)]

        original_random = random.random
        try:
            random.random = lambda: 0.2  # set random.random() to 0.2
            selected = optimizer._select_best_individuals(evaluated)
        finally:
            random.random = original_random

        assert selected == [ind2]

    def test_iterator_stop_iteration(self, optimizer):
        optimizer.producer_finished.set()
        with pytest.raises(StopIteration):
            next(optimizer)

    def test_terator_is_empty(self, optimizer):
        optimizer.producer_finished.set()
        assert optimizer.is_empty()
