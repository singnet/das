from unittest.mock import MagicMock, patch

import grpc
import pytest

import hyperon_das._grpc.common_pb2 as common_pb2
import hyperon_das._grpc.distributed_algorithm_node_pb2 as grpc_pb2
from hyperon_das.query_answer import QueryAnswer
from hyperon_das.service_bus.proxy import (
    BusCommandProxy,
    DistributedAlgorithmNodeManager,
    DistributedAlgorithmNodeServicer,
)
from hyperon_das.service_clients.pattern_matching_query import PatternMatchingQueryProxy


class DummyProxy(BusCommandProxy):
    def __init__(self):
        super().__init__(command="cmd", args=[])
        self.processed_msgs = []

    def pack_command_line_args(self):
        pass

    def process_message(self, msg):
        self.processed_msgs.append(msg)


class TestBaseCommandProxy:
    def test_setup_proxy_node_raises_if_no_port(self):
        proxy = DummyProxy()
        proxy.proxy_port = 0
        with pytest.raises(RuntimeError):
            proxy.setup_proxy_node()

    @patch('hyperon_das.service_bus.proxy.DistributedAlgorithmNodeManager')
    def test_setup_proxy_node_with_requestor_id(self, MockManager):
        proxy = DummyProxy()
        proxy.proxy_port = 1111
        proxy.requestor_id = "host:9999"
        instance = MockManager.return_value
        proxy.setup_proxy_node(server_id="srv")
        expected_node_id = "host:1111"
        MockManager.assert_called_with(node_id=expected_node_id, server_id="srv", proxy=proxy)
        instance.start.assert_called_once()

    @patch('hyperon_das.service_bus.proxy.DistributedAlgorithmNodeManager')
    def test_setup_proxy_node_with_client_override(self, MockManager):
        proxy = DummyProxy()
        proxy.proxy_port = 2222
        instance = MockManager.return_value
        proxy.setup_proxy_node(client_id="cli:1", server_id="srv2")
        MockManager.assert_called_with(node_id="cli:1", server_id="srv2", proxy=proxy)
        assert instance.peer_id == "srv2"
        instance.start.assert_called_once()

    @patch('hyperon_das.service_bus.port_pool.PortPool.return_port')
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


class TestPatternMatchingQueryProxy:
    @pytest.fixture(autouse=True)
    def proxy(self):
        return PatternMatchingQueryProxy(tokens=["x"], context="c")

    def test_finished_logic(self, proxy):
        proxy.abort_flag = False
        proxy.command_finished_flag = False
        assert not proxy.finished()

        proxy.abort_flag = True
        assert proxy.finished()

        proxy.abort_flag = False
        proxy.command_finished_flag = True
        assert proxy.finished()

    def test_pop_behaviour(self, proxy):
        assert proxy.pop() is None
        proxy.abort_flag = True
        assert isinstance(proxy.pop(), QueryAnswer)
        proxy.abort_flag = False
        assert proxy.pop() is None
        qa = QueryAnswer(handle="h", importance=0)
        proxy.answer_queue.put(qa)
        assert proxy.pop() is qa

    def test_process_message_and_counters(self, proxy):
        base = QueryAnswer(handle="h0", importance=0.0)
        tokens = base.tokenize()
        msgs = [PatternMatchingQueryProxy.ANSWER_BUNDLE, tokens, PatternMatchingQueryProxy.COUNT]
        proxy.process_message(msgs)
        assert proxy.answer_count == 1
        assert isinstance(proxy.answer_queue.get(), QueryAnswer)
        # finished
        proxy.process_message([PatternMatchingQueryProxy.FINISHED])
        assert proxy.command_finished_flag
        # abort
        proxy2 = PatternMatchingQueryProxy(tokens=["x"], context="c")
        proxy2.process_message([PatternMatchingQueryProxy.ABORT])
        assert proxy2.abort_flag


class TestDistributedAlgorithmNodeManager:
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
        mgr = DistributedAlgorithmNodeManager(node_id="host:1234", server_id="srv", proxy=None)
        mgr.start()
        stub_grpc.add_insecure_port.assert_called_with("host:1234")
        stub_grpc.start.assert_called_once()

    def test_stop_and_wait(self, stub_grpc):
        mgr = DistributedAlgorithmNodeManager(node_id="h:1", server_id="s", proxy=None)
        mgr.server = stub_grpc
        mgr.stop(grace=5)
        mgr.wait_for_termination()
        stub_grpc.stop.assert_called_with(5)
        stub_grpc.wait_for_termination.assert_called_once()


class TestDistributedAlgorithmNodeServicer:
    @pytest.fixture(autouse=True)
    def servicer(self):
        proxy = MagicMock()
        return DistributedAlgorithmNodeServicer(proxy)

    def test_ping(self, servicer):
        ack = servicer.ping()
        assert not ack.error
        assert ack.msg == "ack"

    def test_execute_message_dispatch(self, servicer):
        servicer.proxy = MagicMock()
        msg = grpc_pb2.MessageData(command="query_answer_tokens_flow", args=["arg1"])
        resp = servicer.execute_message(msg)
        assert isinstance(resp, common_pb2.Empty)
        servicer.proxy.process_message.assert_called_with(["arg1"])

    def test_execute_message_ignores_other(self, servicer):
        servicer.proxy = MagicMock()
        msg = grpc_pb2.MessageData(command="unrelated", args=["arg2"])
        servicer.execute_message(msg)
        servicer.proxy.process_message.assert_not_called()
