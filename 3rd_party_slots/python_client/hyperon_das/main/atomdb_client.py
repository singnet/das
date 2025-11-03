import argparse

from hyperon_das.commons.helpers import tokenize_preserve_quotes
from hyperon_das.service_bus.service_bus import ServiceBusSingleton
from hyperon_das.service_clients import AtomDBProxy


def parse_arguments():
    parser = argparse.ArgumentParser(description='Python AtomDB client')

    parser.add_argument("--client-id", required=True, help="<IP:PORT>")
    parser.add_argument("--peer-id", required=True, help="<IP:PORT>")
    parser.add_argument("--action", required=True, type=str)
    parser.add_argument("--tokens", required=True, type=str)

    return parser.parse_args()


def atomdb_client(client_id: str = None, peer_id: str = None, action: str = None, tokens: list[str] | str = None):
    if any([client_id, peer_id, action, tokens]):
        if not client_id:
            raise ValueError("client_id is required")
        if not peer_id:
            raise ValueError("peer_id is required")
        if not action:
            raise ValueError("action is required")
        if not tokens:
            raise ValueError("query_tokens is required")
        if isinstance(tokens, str):
            tokens = tokenize_preserve_quotes(tokens)
    else:
        args = parse_arguments()
        client_id = args.client_id
        peer_id = args.peer_id
        action = args.action
        tokens = tokenize_preserve_quotes(args.tokens)

    proxy = AtomDBProxy()

    service_bus = ServiceBusSingleton(
        host_id=client_id, known_peer=peer_id, port_lower=41200, port_upper=41299
    ).get_instance()

    try:
        service_bus.issue_bus_command(proxy)
    except Exception:
        raise

    try:
        atoms = proxy.build_atoms_from_tokens(tokens)
        handles = proxy.add_atoms(atoms)
        for handle in handles:
            print(f"handle: {handle}")
    except KeyboardInterrupt:
        pass
    finally:
        proxy.graceful_shutdown()


if __name__ == '__main__':
    atomdb_client()
