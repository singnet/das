import pytest
import random
from evolution.optimizer import QueryOptimizerAgent, QueryOptimizerIterator


class DummyQueryAnswer:
    def __init__(self, text="dummy"):
        self.text = text
        self.handles = ["handle1", "handle2"]

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
        return []


class DummyAttentionBrokerGateway:
    def __init__(self, config):
        self.config = config

    def correlate(self, single_answer):
        pass

    def stimulate(self, handle_count):
        pass


def dummy_handle_fitness_function(name):
    def fitness(db, query_answer):
        return random.random()
    return fitness


def write_config_file(tmp_path, config_data):
    config_file = tmp_path / "config.cfg"
    lines = []
    for key, value in config_data.items():
        lines.append(f"{key}={value}")
    config_file.write_text("\n".join(lines))
    return str(config_file)


@pytest.fixture(autouse=True)
def patch_dependencies(monkeypatch):
    monkeypatch.setattr("optimizer.DASNode", DummyDASNode)
    monkeypatch.setattr("optimizer.RedisMongoDB", DummyRedisMongoDB)
    monkeypatch.setattr("optimizer.handle_fitness_function", dummy_handle_fitness_function)
    monkeypatch.setattr("optimizer.AttentionBrokerGateway", DummyAttentionBrokerGateway)


class TestQueryOptimizer:
    @pytest.fixture
    def valid_config_path(self, tmp_path):
        config_data = {
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
        return write_config_file(tmp_path, config_data)

    @pytest.fixture
    def invalid_config_path(self, tmp_path):
        config_data = {
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
        return write_config_file(tmp_path, config_data)

    def test_query_optimizer_agent_success(self, valid_config_path):
        agent = QueryOptimizerAgent(valid_config_path)
        iterator = agent.optimize("token1 token2")
        assert isinstance(iterator, QueryOptimizerIterator)
        assert iterator.query_tokens == ["token1", "token2"]

    def test_query_optimizer_agent_invalid_config(self, invalid_config_path):
        with pytest.raises(ValueError):
            QueryOptimizerAgent(invalid_config_path)

    def test_query_optimizer_iterator_iteration(self, valid_config_path):
        iterator = QueryOptimizerIterator(
            query_tokens=["token"],
            max_query_answers=2,
            max_generations=1,
            population_size=2,
            qtd_selected_for_attention_update=1,
            selection_method="dummy",
            fitness_function="dummy",
            attention_broker_server_id="localhost:1234",
            query_agent_node_id="node1",
            query_agent_server_id="server1",
            mongo_hostname="localhost",
            mongo_port=27017,
            mongo_username="user",
            mongo_password="pass",
            redis_hostname="localhost",
            redis_port=6379,
            redis_cluster=False,
            redis_ssl=False,
        )
        results = [qa.to_string() for qa in iterator]

        assert len(results) == 2
        assert results == ["answer1", "answer2"]

    def test_query_optimizer_iterator_is_empty(self, valid_config_path):
        iterator = QueryOptimizerIterator(
            query_tokens=["token"],
            max_query_answers=1,
            max_generations=1,
            population_size=1,
            qtd_selected_for_attention_update=1,
            selection_method="dummy",
            fitness_function="dummy",
            attention_broker_server_id="localhost:1234",
            query_agent_node_id="node1",
            query_agent_server_id="server1",
            mongo_hostname="localhost",
            mongo_port=27017,
            mongo_username="user",
            mongo_password="pass",
            redis_hostname="localhost",
            redis_port=6379,
            redis_cluster=False,
            redis_ssl=False,
        )
        list(iterator)
        assert iterator.is_empty()
