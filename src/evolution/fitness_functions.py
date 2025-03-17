from hyperon_das_atomdb.exceptions import AtomDoesNotExist
from typing import ClassVar


def multiply_strengths(atom_db, handles: list[str]) -> float:
    product = 1.0
    found_strength = False
    for handle in handles:
        try:
            atom = atom_db.get_atom(handle)
        except AtomDoesNotExist as e:
            print(f'Error: {e}')
            continue
        if (custom_attributes := atom.custom_attributes):
            if (strength := custom_attributes.get('strength')):
                product *= strength
                found_strength = True
    return product if found_strength else 0.0


class FitnessFunctions:
    _fitness_functions: ClassVar[dict[str, callable]] = {
        "multiply_strengths": multiply_strengths
    }

    @classmethod
    def get(cls, fitness_function_name: str) -> callable:
        if fitness_function_name not in cls._fitness_functions:
            raise ValueError("Unknown fitness function")
        return cls._fitness_functions[fitness_function_name]
