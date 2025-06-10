import pytest

from hyperon_das.port_pool import PortPool


def test_initialize_invalid_range():
    with pytest.raises(ValueError) as exc:
        PortPool.initialize_statics(10, 5)
    assert "Invalid port limits" in str(exc.value)


def test_get_port_before_initialization_raises():
    with pytest.raises(ValueError) as exc:
        PortPool.get_port()
    assert "not initialized" in str(exc.value)


def test_return_port_before_initialization_raises():
    with pytest.raises(ValueError) as exc:
        PortPool.return_port(1234)
    assert "not initialized" in str(exc.value)


def test_initialize_and_get_until_exhausted():
    PortPool.initialize_statics(8000, 8002)
    ports = {PortPool.get_port() for _ in range(3)}
    assert ports == {8000, 8001, 8002}
    with pytest.raises(ValueError) as exc:
        PortPool.get_port()
    assert "Unable to get available PORT number" in str(exc.value)


def test_return_outside_range_raises():
    PortPool.initialize_statics(8000, 8002)
    with pytest.raises(ValueError) as exc:
        PortPool.return_port(7999)
    assert "Port out of the available range" in str(exc.value)


def test_return_and_retrieve():
    PortPool.initialize_statics(9000, 9000)
    port = PortPool.get_port()
    assert port == 9000

    with pytest.raises(ValueError):
        PortPool.get_port()

    PortPool.return_port(9000)
    assert PortPool.get_port() == 9000


def test_reinitialize_resets_pool():
    PortPool.initialize_statics(1, 2)
    _ = PortPool.get_port()
    PortPool.initialize_statics(3, 4)
    ports = {PortPool.get_port() for _ in range(2)}
    assert ports == {3, 4}
