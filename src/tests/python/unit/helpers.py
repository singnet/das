from random import choice
from string import ascii_lowercase

from hyperon_das import DistributedAtomSpace


def load_n_random_links_by_type(
    das: DistributedAtomSpace, n: int, type: str = "Inheritance"
) -> None:
    for name in ["".join([choice(ascii_lowercase) for c in range(5)]) for i in range(n)]:
        das.add_link(
            {
                "type": type,
                "targets": [
                    {"type": "Concept", "name": "human"},
                    {"type": "Concept", "name": name},
                ],
            }
        )
