import time
from concurrent import futures

import grpc
import pytest

import hyperon_das.grpc.attention_broker_pb2_grpc as ab_grpc
import hyperon_das.grpc.common_pb2 as common
from hyperon_das.cache.attention_broker_gateway import AttentionBrokerGateway

HOST = "localhost"
PORT = 27000
SYSTEM_PARAMETERS = {"attention_broker_hostname": HOST, "attention_broker_port": PORT}

RECEIVED = None


class AttentionBrokerMock(ab_grpc.AttentionBrokerServicer):
    def ping(self, request, context):
        return common.Ack(error=0, msg="OK")

    def correlate(self, request, context):
        global RECEIVED
        RECEIVED = request.list
        return common.Ack(error=0, msg="OK")

    def stimulate(self, request, context):
        global RECEIVED
        RECEIVED = request.map
        return common.Ack(error=0, msg="OK")


def server_up():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    ab_grpc.add_AttentionBrokerServicer_to_server(AttentionBrokerMock(), server)
    server.add_insecure_port(f"[::]:{PORT}")
    server.start()
    return server


def server_down(server):
    server.stop(1)
    time.sleep(1)


class TestAttentionBrokerGateway:
    def test_creation(self):
        with pytest.raises(ValueError):
            AttentionBrokerGateway({})
        with pytest.raises(ValueError):
            AttentionBrokerGateway({"attention_broker_hostname": "localhost"})
        with pytest.raises(ValueError):
            AttentionBrokerGateway({"attention_broker_port": 27000})
        grpc_server = server_up()
        AttentionBrokerGateway(SYSTEM_PARAMETERS)
        server_down(grpc_server)

    def test_ping(self):
        grpc_server = server_up()
        gateway = AttentionBrokerGateway(SYSTEM_PARAMETERS)
        assert gateway.ping() == "OK"
        server_down(grpc_server)

    def _check_correlate(self, gateway, message):
        global RECEIVED
        assert gateway.correlate(message) == "OK"
        assert list(RECEIVED) == list(message)

    def _check_stimulate(self, gateway, stimuli):
        global RECEIVED
        message = {handle: count for handle, count in stimuli}
        assert gateway.stimulate(message) == "OK"
        assert len(RECEIVED) == len(message)
        for key in RECEIVED:
            assert RECEIVED[key] == message[key]

    def test_correlate(self):
        grpc_server = server_up()
        gateway = AttentionBrokerGateway(SYSTEM_PARAMETERS)
        with pytest.raises(ValueError):
            self._check_correlate(gateway, None)
        self._check_correlate(gateway, [])
        self._check_correlate(gateway, ["h1"])
        self._check_correlate(gateway, ["h1", "h2"])
        self._check_correlate(gateway, ["h1", "h2", "h2"])
        self._check_correlate(gateway, ["h2", "h1", "h2"])
        self._check_correlate(gateway, set(["h1", "h2", "h2"]))
        server_down(grpc_server)

    def test_stimulate(self):
        grpc_server = server_up()
        gateway = AttentionBrokerGateway(SYSTEM_PARAMETERS)
        with pytest.raises(ValueError):
            self._check_correlate(gateway, None)
        self._check_stimulate(gateway, [])
        self._check_stimulate(gateway, [("h1", 1)])
        self._check_stimulate(gateway, [("h1", 1), ("h2", 1)])
        self._check_stimulate(gateway, [("h1", 1), ("h2", 2)])
        server_down(grpc_server)
