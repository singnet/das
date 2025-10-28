import pytest
from unittest.mock import MagicMock, patch
from hyperon_das.service_bus.service_bus import ServiceBus, ServiceBusSingleton
from hyperon_das.service_clients import PatternMatchingQueryProxy
from hyperon_das.service_bus.port_pool import PortPool
from hyperon_das.distributed_algorithm_node.bus_node import BusNode


class TestServiceBus:
    @pytest.fixture
    def mock_port_pool(self):
        with patch.object(PortPool, 'initialize_statics') as init_mock, patch.object(PortPool, 'get_port', return_value=12345) as port_mock:
            yield init_mock, port_mock

    @pytest.fixture
    def mock_bus_node(self):
        bus_node = MagicMock(spec=BusNode)
        bus_node.send_bus_command = MagicMock()
        return bus_node

    @pytest.fixture
    def mock_proxy(self):
        proxy = MagicMock(spec=PatternMatchingQueryProxy)
        proxy.command = 'PATTERN_MATCHING_QUERY'
        proxy.args = ['arg1', 'arg2']
        proxy.setup_proxy_node = MagicMock()
        proxy.proxy_node = MagicMock()
        return proxy

    def test_service_bus_issues_command(self, mock_port_pool, mock_bus_node, mock_proxy):
        sb = ServiceBus("node1", "peer1", ["PATTERN_MATCHING_QUERY"], 3000, 4000)
        sb.issue_bus_command(mock_proxy)

        assert mock_proxy.requestor_id == "node1"
        assert mock_proxy.serial == 1
        assert mock_proxy.proxy_port == 12345
        mock_proxy.setup_proxy_node.assert_called_once()

    def test_service_bus_raises_if_no_port(self, mock_port_pool, mock_bus_node, mock_proxy):
        _, port_mock = mock_port_pool
        port_mock.return_value = 0

        sb = ServiceBus("node1", "peer1", ["PATTERN_MATCHING_QUERY"], 3000, 4000)

        with pytest.raises(RuntimeError, match="No port is available"):
            sb.issue_bus_command(mock_proxy)

    def test_service_bus_singleton_behavior(self):
        instance1 = ServiceBusSingleton("host1", "peer1", 1000, 2000)
        instance2 = ServiceBusSingleton("host2", "peer2", 3000, 4000)

        assert instance1 is instance2
        assert instance1.get_instance() is instance2.get_instance()
        assert isinstance(instance1.get_instance(), ServiceBus)