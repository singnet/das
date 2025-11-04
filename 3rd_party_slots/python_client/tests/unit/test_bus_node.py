from unittest.mock import MagicMock

import pytest

from hyperon_das.distributed_algorithm_node.bus_node import BusNode


class TestBusNode:
    def test_init(self):
        mock_bus = MagicMock()
        node = BusNode('node1', ['cmd1', 'cmd2'], 'peer1')
        node._bus = mock_bus
        for cmd in ['cmd1', 'cmd2']:
            node._bus.add(cmd)
            node._bus.set_ownership(cmd, 'peer1')

        assert node.node_id == 'node1'
        assert mock_bus.add.called
        assert mock_bus.add.call_args_list == [((cmd,),) for cmd in ['cmd1', 'cmd2']]
        assert mock_bus.set_ownership.call_args_list == [((cmd, 'peer1'),) for cmd in ['cmd1', 'cmd2']]

    def test_send_bus_command_no_owner(self):
        mock_bus = MagicMock()
        mock_bus.get_ownership.return_value = ""
        node = BusNode('node1', [], '')
        node._bus = mock_bus
        with pytest.raises(RuntimeError) as excinfo:
            node.send_bus_command('cmd1', ['arg1'])
        assert str(excinfo.value) == "Bus: no owner is defined for command <cmd1>"

    def test_send_bus_command_with_owner(self):
        mock_bus = MagicMock()
        mock_bus.get_ownership.return_value = 'peer1'
        mock_send = MagicMock()
        node = BusNode('node1', [])
        node._bus = mock_bus
        node.send = mock_send
        node.send_bus_command('cmd1', ['arg1'])
        mock_send.assert_called_with('cmd1', ['arg1'], 'peer1')
