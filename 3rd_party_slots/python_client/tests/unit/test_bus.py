import pytest

from hyperon_das.bus_node import Bus, BusCommand


def test_bus_command_enum():
    assert isinstance(BusCommand.PATTERN_MATCHING_QUERY, BusCommand)
    assert BusCommand.PATTERN_MATCHING_QUERY.value == "pattern_matching_query"


def test_add_and_contains():
    bus = Bus()
    assert not bus.contains("snet")
    bus.add("snet")
    assert bus.contains("snet")


def test_set_and_get_ownership():
    bus = Bus()
    bus.add("cmd")

    with pytest.raises(RuntimeError) as exc:
        bus.get_ownership("unknown")
    assert "unknown command" in str(exc.value)

    with pytest.raises(RuntimeError) as exc2:
        bus.set_ownership("undef", "node1")
    assert "is not defined" in str(exc2.value)

    bus.set_ownership("cmd", "node1")
    assert bus.get_ownership("cmd") == "node1"

    bus.set_ownership("cmd", "node1")
    assert bus.get_ownership("cmd") == "node1"

    with pytest.raises(RuntimeError) as exc3:
        bus.set_ownership("cmd", "node2")
    assert "already assigned to node1" in str(exc3.value)


def test_to_string_formatting():
    bus = Bus()
    bus.add("a")
    bus.add("b")
    bus.set_ownership("b", "nodeB")
    s = bus.to_string()

    assert s.startswith("{") and s.endswith("}")

    assert "a" in s
    assert "b:nodeB" in s


def test_equality_and_copy():
    bus1 = Bus()
    bus1.add("x")
    bus1.set_ownership("x", "n1")

    bus2 = Bus(bus1)
    assert bus1 == bus2

    bus1.add("y")
    assert bus1 != bus2

    bus3 = Bus()
    ret = bus3 + "z"
    assert isinstance(ret, Bus)
    assert ret.contains("z")
    assert bus3.contains("z")


def test_add_after_ownership_remains_error():
    bus = Bus()
    bus.add("alpha")
    bus.set_ownership("alpha", "nodeA")

    with pytest.raises(RuntimeError) as exc:
        bus.add("alpha")
    assert "is already assigned to nodeA" in str(exc.value)
