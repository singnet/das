from typing import Set

import pytest
from hyperon_das_atomdb.database import NodeT

from hyperon_das.cache.attention_broker_gateway import AttentionBrokerGateway
from hyperon_das.cache.cache_controller import CacheController
from hyperon_das.context import Context
from hyperon_das.utils import QueryAnswer

SYSTEM_PARAMETERS = {
    "attention_broker_hostname": None,
    "attention_broker_port": None,
    "cache_enabled": False,
}


class AttentionBrokerGatewayMock(AttentionBrokerGateway):
    def __init__(self):
        self.handle_set_list = []
        pass

    def correlate(self, handle_set: Set[str]) -> str:
        self.handle_set_list.append(handle_set)

    def stimulate(self, handle_count: Set[str]) -> str:
        self.handle_count = handle_count


class TestCacheController:
    def _build_controller(self):
        params = SYSTEM_PARAMETERS.copy()
        controller = CacheController(params)
        params["cache_enabled"] = True
        controller.attention_broker = AttentionBrokerGatewayMock()
        return controller

    def test_creation(self):
        # Relevant to test dependency on AttentionBrokerGateway
        controller = CacheController({})
        assert not controller.enabled()  # assert default == False
        with pytest.raises(ValueError):
            controller = CacheController({"cache_enabled": True})
        controller = CacheController({"cache_enabled": False})
        assert not controller.enabled()

    def test_get_atom(self):
        controller = CacheController({})
        assert controller.get_atom("blah") is None

    def test_add_context(self):
        controller = self._build_controller()
        node = NodeT(type="Context", name="blah")
        node.handle = node._id = "h1"
        context = Context(
            node,
            [
                [
                    QueryAnswer({"handle": "h1"}, None),
                    QueryAnswer({"handle": "h2"}, None),
                ],
                [
                    QueryAnswer({"handle": "h1"}, None),
                    QueryAnswer({"handle": "h3"}, None),
                ],
            ],
        )
        controller.add_context(context)
        broker = controller.attention_broker

        assert len(controller.attention_broker.handle_set_list) == 4
        assert broker.handle_set_list[0] == set(["h1"])
        assert broker.handle_set_list[1] == set(["h2"])
        assert broker.handle_set_list[2] == set(["h1"])
        assert broker.handle_set_list[3] == set(["h3"])
        assert len(broker.handle_count) == 2
        assert broker.handle_count["h1"] == 1
        assert broker.handle_count["h3"] == 1

    def test_regard_query_answer(self):
        controller = self._build_controller()

        query_answer_1 = QueryAnswer({"handle": "h7"}, None)
        query_answer_2 = QueryAnswer(
            {
                "handle": "h1",
                "targets": [
                    {"handle": "h8"},
                    {"handle": "h9"},
                ],
            },
            None,
        )
        query_answer_3 = QueryAnswer(
            {
                "handle": "h1",
                "targets": [
                    {"handle": "h2"},
                    {
                        "handle": "h2",
                        "targets": [
                            {"handle": "h4"},
                            {"handle": "h1"},
                        ],
                    },
                    {
                        "handle": "h5",
                        "targets": [
                            {"handle": "h1"},
                            {"handle": "h6"},
                        ],
                    },
                    {"handle": "h3"},
                ],
            },
            None,
        )

        controller.regard_query_answer([query_answer_1, query_answer_2, query_answer_3])
        broker = controller.attention_broker

        assert len(controller.attention_broker.handle_set_list) == 3
        assert broker.handle_set_list[0] == set(["h7"])
        assert broker.handle_set_list[1] == set(["h1", "h8", "h9"])
        assert broker.handle_set_list[2] == set(["h1", "h2", "h3", "h4", "h5", "h6"])

        all_handles = set()
        for handle_set in broker.handle_set_list:
            all_handles.update(handle_set)
        assert len(broker.handle_count) == len(all_handles)
        assert broker.handle_count["h1"] == 4
        assert broker.handle_count["h2"] == 2
        assert broker.handle_count["h3"] == 1
        assert broker.handle_count["h4"] == 1
        assert broker.handle_count["h5"] == 1
        assert broker.handle_count["h6"] == 1
        assert broker.handle_count["h7"] == 1
        assert broker.handle_count["h8"] == 1
        assert broker.handle_count["h9"] == 1
