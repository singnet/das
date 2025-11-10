import threading
import time

from hyperon_das.commons.atoms import Atom, Link, Node
from hyperon_das.distributed_algorithm_node.bus_node import BusCommand
from hyperon_das.logger import log
from hyperon_das.service_clients.base import BaseProxy

MAX_ARGS_BYTES = 3 * 1024 * 1024  # 3 MB


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
        Serialize atoms into args, split args into chunks if necessary to respect the
        message size limit, and then send each chunk to the remote peer.
        """
        with self._lock:
            full_args = []
            handles = []
            chunks = []
            current_chunk = []
            current_size = 0
            temp_atom_args = []
            atoms_count = 0

            for atom in atoms:
                handles.append(atom.handle())

                temp_atom_args.clear()
                atom.tokenize(temp_atom_args)

                if isinstance(atom, Node):
                    atom_type = "NODE"
                elif isinstance(atom, Link):
                    atom_type = "LINK"
                else:
                    log.error(f"Invalid Atom Type: {type(atom)}")
                    raise ValueError("Invalid Atom Type")

                full_args.append(atom_type)
                full_args.extend(temp_atom_args)

                args_size = sum(len(arg.encode('utf-8')) for arg in ([atom_type] + temp_atom_args))

                if current_size + args_size > MAX_ARGS_BYTES and current_chunk:
                    chunks.append((current_chunk, atoms_count))
                    current_chunk = []
                    current_size = 0
                    atoms_count = 0

                current_chunk.extend([atom_type] + temp_atom_args)
                atoms_count += 1
                current_size += args_size

            if current_chunk:
                chunks.append((current_chunk, atoms_count))

            log.debug(f"Sending {len(atoms)} atoms in {len(chunks)} chunk(s).")

            for i, (chunk_args, atoms_count) in enumerate(chunks):
                log.debug(f"Sending chunk {i+1}/{len(chunks)} with {atoms_count} atoms.")
                try:
                    self.to_remote_peer(self.ADD_ATOMS, chunk_args)
                    time.sleep(2)  # Delay to avoid overwhelming the receiver
                except Exception as e:
                    log.error(f"Failed to send chunk {i+1}/{len(chunks)}: {e}")
                    raise RuntimeError(f"Failed to send data chunk {i+1} to remote peer.") from e

            return handles
