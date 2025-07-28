import pytest
import grpc

from unittest.mock import MagicMock, patch

import hyperon_das._grpc.atom_space_node_pb2 as grpc_pb2
import hyperon_das._grpc.common_pb2 as common_pb2

from hyperon_das.proxy import (
    BaseCommandProxy,
    PatternMatchingQueryHandler,
    AtomSpaceNodeManager,
    AtomSpaceNodeServicer
)
from hyperon_das.query_answer import QueryAnswer


class DummyProxy(BaseCommandProxy):
    def __init__(self):
        super().__init__(command="cmd", args=[])
        self.processed_msgs = []

    def process_message(self, msg):
        self.processed_msgs.append(msg)


class TestBaseCommandProxy:
    def test_setup_proxy_node_raises_if_no_port(self):
        proxy = DummyProxy()
        proxy.proxy_port = 0
        with pytest.raises(RuntimeError):
            proxy.setup_proxy_node()

    @patch('hyperon_das.proxy.AtomSpaceNodeManager')
    def test_setup_proxy_node_with_requestor_id(self, MockManager):
        proxy = DummyProxy()
        proxy.proxy_port = 1111
        proxy.requestor_id = "host:9999"
        instance = MockManager.return_value
        proxy.setup_proxy_node(server_id="srv")
        expected_node_id = "host:1111"
        MockManager.assert_called_with(node_id=expected_node_id, server_id="srv", handler=proxy)
        instance.start.assert_called_once()

    @patch('hyperon_das.proxy.AtomSpaceNodeManager')
    def test_setup_proxy_node_with_client_override(self, MockManager):
        proxy = DummyProxy()
        proxy.proxy_port = 2222
        instance = MockManager.return_value
        proxy.setup_proxy_node(client_id="cli:1", server_id="srv2")
        MockManager.assert_called_with(node_id="cli:1", server_id="srv2", handler=proxy)
        assert instance.peer_id == "srv2"
        instance.start.assert_called_once()

    @patch('hyperon_das.port_pool.PortPool.return_port')
    def test_graceful_shutdown_calls_node_and_returns_port(self, mock_return):
        proxy = DummyProxy()
        proxy.proxy_port = 3333
        proxy.proxy_node = MagicMock()
        proxy.graceful_shutdown()
        proxy.proxy_node.stop.assert_called_once()
        proxy.proxy_node.wait_for_termination.assert_called_once()
        mock_return.assert_called_with(3333)

    def test_graceful_shutdown_no_port_logs(self):
        proxy = DummyProxy()
        proxy.proxy_port = 0
        # should not raise
        proxy.graceful_shutdown()


class TestPatternMatchingQueryHandler:
    @pytest.fixture(autouse=True)
    def handler(self):
        return PatternMatchingQueryHandler(tokens=["x"], context="c")

    def test_finished_logic(self, handler):
        handler.abort_flag = False
        handler.answer_flow_finished = False
        handler.count_flag = False
        assert not handler.finished()

        handler.abort_flag = True
        assert handler.finished()

        handler.abort_flag = False
        handler.answer_flow_finished = True
        handler.count_flag = True
        assert handler.finished()

    def test_pop_behaviour(self, handler):
        handler.count_flag = True
        assert handler.pop() is None
        handler.count_flag = False
        handler.abort_flag = True
        assert handler.pop() is None
        handler.abort_flag = False
        assert handler.pop() is None
        qa = QueryAnswer(handle="h", importance=0)
        handler.answer_queue.put(qa)
        assert handler.pop() is qa

    def test_process_message_and_counters(self, handler):
        base = QueryAnswer(handle="h0", importance=0.0)
        tokens = base.tokenize()
        msgs = [PatternMatchingQueryHandler.ANSWER_BUNDLE, tokens, PatternMatchingQueryHandler.COUNT]
        handler.process_message(msgs)
        assert handler.answer_count == 1
        assert isinstance(handler.answer_queue.get(), QueryAnswer)
        # finished
        handler.process_message([PatternMatchingQueryHandler.FINISHED])
        assert handler.answer_flow_finished
        # abort
        handler2 = PatternMatchingQueryHandler(tokens=["x"], context="c")
        handler2.process_message([PatternMatchingQueryHandler.ABORT])
        assert handler2.abort_flag


class TestAtomSpaceNodeManager:
    @pytest.fixture(autouse=True)
    def stub_grpc(self, monkeypatch):
        fake_server = MagicMock()
        fake_server.add_insecure_port = MagicMock()
        fake_server.start = MagicMock()
        fake_server.stop = MagicMock()
        fake_server.wait_for_termination = MagicMock()
        monkeypatch.setattr(grpc, 'server', lambda executor: fake_server)
        return fake_server

    def test_start_adds_port_and_starts(self, stub_grpc):
        mgr = AtomSpaceNodeManager(node_id="host:1234", server_id="srv", handler=None)
        mgr.start()
        stub_grpc.add_insecure_port.assert_called_with("host:1234")
        stub_grpc.start.assert_called_once()

    def test_stop_and_wait(self, stub_grpc):
        mgr = AtomSpaceNodeManager(node_id="h:1", server_id="s", handler=None)
        mgr.server = stub_grpc
        mgr.stop(grace=5)
        mgr.wait_for_termination()
        stub_grpc.stop.assert_called_with(5)
        stub_grpc.wait_for_termination.assert_called_once()


class TestAtomSpaceNodeServicer:
    @pytest.fixture(autouse=True)
    def servicer(self):
        handler = MagicMock()
        return AtomSpaceNodeServicer(handler)

    def test_ping(self, servicer):
        ack = servicer.ping()
        assert not ack.error
        assert ack.msg == "ack"

    def test_execute_message_dispatch(self, servicer):
        servicer.handler = MagicMock()
        msg = grpc_pb2.MessageData(command="query_answer_tokens_flow", args=["arg1"])
        resp = servicer.execute_message(msg)
        assert isinstance(resp, common_pb2.Empty)
        servicer.handler.process_message.assert_called_with(["arg1"])

    def test_execute_message_ignores_other(self, servicer):
        servicer.handler = MagicMock()
        msg = grpc_pb2.MessageData(command="unrelated", args=["arg2"])
        servicer.execute_message(msg)
        servicer.handler.process_message.assert_not_called()
