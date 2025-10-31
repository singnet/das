from hyperon_das.service_bus.service_bus import ServiceBusSingleton
from hyperon_das.service_clients import AtomDBProxy
from hyperon_das.hasher import Hasher
from hyperon_das.commons.atoms import Node, Link
from hyperon_das.commons.properties import Properties
from .utils import AtomDecoder


def test_atomdb_proxy_client():
    service_bus = ServiceBusSingleton(
        host_id="0.0.0.0:9001",
        known_peer="0.0.0.0:40006",
        port_lower=41200,
        port_upper=41299
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
        "NODE", "Symbol", "false", "3", "is_literal", "bool", "false", "Similarity",
        "NODE", "Symbol", "false", "3", "is_literal", "bool", "true",  '"human"',
        "NODE", "Symbol", "false", "3", "is_literal", "bool", "true",  '"monkey"',
        "LINK", "Expression", "true",  "0", "3", node_handle1, node_handle2, node_handle3
    ]

    atoms = proxy.build_atoms_from_tokens(atoms_tokens)

    assert node1 == atoms[0]
    assert node2 == atoms[1]
    assert node3 == atoms[2]
    assert link == atoms[3]

    proxy.add_atoms(atoms)
    
    decoder = AtomDecoder()
    
    node_db1 = decoder.get_atom("adf3feaaf9c5ed3589979f3c6fef19f2")
    node_db2 = decoder.get_atom(node2.handle())
    node_db3 = decoder.get_atom(node3.handle())
    link_db = decoder.get_atom(link.handle())
    
    assert node1 == node_db1
    assert node2 == node_db2
    assert node3 == node_db3
    assert link == link_db
