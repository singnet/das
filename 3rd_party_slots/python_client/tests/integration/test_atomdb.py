import concurrent.futures
import uuid

from hyperon_das.commons.atoms import Link, Node
from hyperon_das.commons.properties import Properties
from hyperon_das.hasher import Hasher
from hyperon_das.service_bus.service_bus import ServiceBusSingleton
from hyperon_das.service_clients import AtomDBProxy

from .utils import AtomDecoder


def test_atomdb_proxy_simple_client():
    service_bus = ServiceBusSingleton(
        host_id="0.0.0.0:9001", known_peer="0.0.0.0:40006", port_lower=41200, port_upper=41299
    ).get_instance()

    proxy = AtomDBProxy()
    service_bus.issue_bus_command(proxy)

    node_handle1 = Hasher.node_handle("Symbol", 'Similarity')
    node_handle2 = Hasher.node_handle("Symbol", '"human"')
    node_handle3 = Hasher.node_handle("Symbol", '"monkey"')

    attr1 = Properties()
    attr1['is_literal'] = False
    attr2 = Properties()
    attr2['is_literal'] = True

    node1 = Node(type="Symbol", name='Similarity', is_toplevel=False, custom_attributes=attr1)
    node2 = Node(type="Symbol", name='"human"', is_toplevel=False, custom_attributes=attr2)
    node3 = Node(type="Symbol", name='"monkey"', is_toplevel=False, custom_attributes=attr2)
    link = Link(type="Expression", targets=[node_handle1, node_handle2, node_handle3], is_toplevel=True)

    atoms_tokens = [
        "NODE",
        "Symbol",
        "false",
        "3",
        "is_literal",
        "bool",
        "false",
        "Similarity",
        "NODE",
        "Symbol",
        "false",
        "3",
        "is_literal",
        "bool",
        "true",
        '"human"',
        "NODE",
        "Symbol",
        "false",
        "3",
        "is_literal",
        "bool",
        "true",
        '"monkey"',
        "LINK",
        "Expression",
        "true",
        "0",
        "3",
        node_handle1,
        node_handle2,
        node_handle3,
    ]

    atoms = proxy.build_atoms_from_tokens(atoms_tokens)

    assert node1 == atoms[0]
    assert node2 == atoms[1]
    assert node3 == atoms[2]
    assert link == atoms[3]

    proxy.add_atoms(atoms)

    decoder = AtomDecoder()

    node_db1 = decoder.get_atom(node1.handle())
    node_db2 = decoder.get_atom(node2.handle())
    node_db3 = decoder.get_atom(node3.handle())
    link_db = decoder.get_atom(link.handle())

    assert node1 == node_db1
    assert node2 == node_db2
    assert node3 == node_db3
    assert link == link_db


def _build_tokens_for_nodes(node_names):
    tokens = []
    handles = []
    for name in node_names:
        handle = Hasher.node_handle("Symbol", name)
        handles.append(handle)
        tokens.extend(["NODE", "Symbol", "false", "3", "is_literal", "bool", "false", name])
    return tokens, handles


def _client_send_atoms(service_bus, node_names):
    """
    Function executed by each "client". Creates its own proxy, registers it on the service bus, and sends the atoms.
    Returns the list of identifiers for the nodes sent.
    """
    proxy = AtomDBProxy()
    service_bus.issue_bus_command(proxy)

    tokens, handles = _build_tokens_for_nodes(node_names)
    atoms = proxy.build_atoms_from_tokens(tokens)
    proxy.add_atoms(atoms)

    return handles


def test_atomdb_proxy_concurrent_clients():
    """
    This test simulates N simultaneous clients, each sending its atoms to AtomDB through a proxy.
    """
    service_bus = ServiceBusSingleton(
        host_id="0.0.0.0:9001", known_peer="0.0.0.0:40006", port_lower=41200, port_upper=41299
    ).get_instance()

    N = 5
    nodes_per_client = 2

    all_expected_handles = []
    clients_node_lists = []
    for i in range(N):
        node_names = []
        for j in range(nodes_per_client):
            node_names.append(f'"node_client{i}_{j}_{uuid.uuid4().hex}"')
        clients_node_lists.append(node_names)

    with concurrent.futures.ThreadPoolExecutor(max_workers=N) as exe:
        futures = [exe.submit(_client_send_atoms, service_bus, client_nodes) for client_nodes in clients_node_lists]
        for fut in concurrent.futures.as_completed(futures):
            handles = fut.result()
            all_expected_handles.extend(handles)

    decoder = AtomDecoder()

    missing = []
    for h in all_expected_handles:
        atom = decoder.get_atom(h)
        if atom is None:
            missing.append(h)

    assert not missing, f"The following identifiers were not found in the database: {missing}"
