import enum
import math


class FitnessFunctionsTAG(str, enum.Enum):
    MULTIPLY_STRENGTHS = "multiply_strengths"


def handle_fitness_function(tag: str) -> callable:
    if tag == FitnessFunctionsTAG.MULTIPLY_STRENGTHS:
        return multiply_strengths
    else:
        raise ValueError("Unknown fitness function TAG")


def multiply_strengths(atom_db, query) -> float:
    strengths_value = []
    for handle in query.handles:
        atom = atom_db.get_atom(handle)
        if (custom_attributes := atom.custom_attributes):
            if (strength := custom_attributes.get('strength')):
                strengths_value.append(strength)
            # if (truth_value := custom_attributes.get('truth_value')):
            #     strength, confidence = truth_value
            #     strengths_value.append(strength)
    return math.prod(strengths_value)
