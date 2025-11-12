import threading

from hyperon_das.commons.atoms import Atom, Link, Node
from hyperon_das.distributed_algorithm_node.bus_node import BusCommand
from hyperon_das.logger import log
from hyperon_das.service_clients.base import BaseProxy


class AtomDBProxy(BaseProxy):

    ADD_ATOMS = "add_atoms"

    def __init__(self) -> None:
        super().__init__()
        self._lock = threading.Lock()
        self.command = BusCommand.ATOMDB
        log.debug("Created AtomDBProxy")

    def pack_command_line_args(self) -> None:
        self.tokenize(self.args)

    def tokenize(self, output: list[str]) -> None:
        return super().tokenize(output)

    def build_atoms_from_tokens(self, tokens: list[str]):
        """
        Reconstruct Atom objects (Node or Link) from a token stream.

        Args:
            tokens: Token stream containing one or more NODE/LINK blocks
                following the token format in the class docstring.

        Returns:
            A list of `Atom` instances (Node or Link) in the same order they
            appeared in the token stream.
        """
        with self._lock:
            atoms = []
            current = ""
            buffer = []

            def flush():
                if not current:
                    return

                if current == "NODE":
                    atoms.append(Node(tokens=buffer))
                else:  # LINK
                    atoms.append(Link(tokens=buffer))

                buffer.clear()

            for token in tokens:
                if token == "NODE" or token == "LINK":
                    if current:
                        flush()
                    current = token
                else:
                    buffer.append(token)

            if len(buffer) != 0 and current:
                flush()

            return atoms

    def add_atoms(self, atoms: list[Atom]) -> list[str]:
        """
        Serialize atoms, send `ADD_ATOMS` to the remote peer, and return their handles.

        Each atom must implement `tokenize(args: list[str])` and `handle() -> str`.
        The method prepends the atom-type token ("NODE" / "LINK") to each atom's
        token block before sending.

        Args:
            atoms: List of Atom instances to serialize and send.

        Returns:
            List of handles corresponding to the provided atoms, in the same order.

        Raises:
            ValueError: If an atom is neither `Node` nor `Link`.
        """
        args = []
        handles = []
        atom_type = ""

        for atom in atoms:
            atom.tokenize(args)

            if isinstance(atom, Node):
                atom_type = "NODE"
            elif isinstance(atom, Link):
                atom_type = "LINK"
            else:
                log.error("Invalid Atom Type")
                raise ValueError("Invalid Atom Type")

            args.insert(0, atom_type)
            handles.append(atom.handle())

        self.to_remote_peer(self.ADD_ATOMS, args)

        return handles
