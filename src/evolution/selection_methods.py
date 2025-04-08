"""
Module for selecting individuals based on fitness.

This module implements two selection methods:
  - best_fitness: Selects individuals with the highest fitness values.
  - roulette: Selects individuals proportionally to their fitness.

It also provides a function to retrieve the appropriate selection function based on a specified method.
"""

import enum
import random
from typing import Any


class SelectionMethodType(str, enum.Enum):
    BEST_FITNESS = "best_fitness"
    ROULETTE = "roulette"
    # ROULETTE_WITH_REMOVAL = "roulette_with_removal"


def handle_selection_method(method: SelectionMethodType) -> callable:
    """
    Returns the selection function based on the specified method.

    Parameters:
        method (SelectionMethodType): The desired selection method.

    Returns:
        callable: Function that performs the selection of individuals.

    Raises:
        ValueError: If the provided selection method is unknown.
    """
    if method == SelectionMethodType.BEST_FITNESS:
        return best_fitness
    elif method == SelectionMethodType.ROULETTE:
        return roulette
    # elif method == SelectionMethodType.ROULETTE_WITH_REMOVAL:
    #     return roulette_with_removal
    else:
        raise ValueError("Unknown selection method type")


def best_fitness(population: list[tuple[Any, float]], max_individuals: int) -> list[Any]:
    """
    Selects individuals with the highest fitness values.

    Parameters:
        population (list[tuple[Any, float]]): List of tuples (individual, fitness).
        max_individuals (int): Number of individuals to select.

    Returns:
        list[Any]: List of selected individuals.
    """
    selected_individuals = []
    evaluated_sorted = sorted(population, key=lambda x: x[1], reverse=True)

    while evaluated_sorted and len(selected_individuals) < max_individuals:
        try:
            individual = evaluated_sorted.pop(0)
            selected_individuals.append(individual)
        except IndexError as e:
            print(f"Error: {e}")

    return selected_individuals


def roulette(population: list[tuple[Any, float]], max_individuals: int) -> list[Any]:
    """
    Selects individuals based on the roulette wheel method (fitness-proportionate).

    Each individual has a chance of being selected proportional to its fitness.

    Parameters:
        population (list[tuple[Any, float]]): List of tuples (individual, fitness).
        max_individuals (int): Number of individuals to select.

    Returns:
        list[Any]: List of selected individuals.

    Raises:
        ValueError: If the total fitness is zero, making it impossible to calculate probabilities.
    """
    total_fitness = sum(fitness for _, fitness in population)

    if total_fitness == 0:
        print(
            f"Total fitness is zero for population {population}. cannot perform roulette selection.",
            flush=True,
        )
        return []

    selection_probabilities = [fitness / total_fitness for _, fitness in population]
    selected_individuals = set()

    while len(selected_individuals) < min(len(population), max_individuals):
        selection_point = random.random()
        running_sum_of_probabilities = 0
        for individual, selection_probability in zip(population, selection_probabilities):
            running_sum_of_probabilities += selection_probability
            if running_sum_of_probabilities >= selection_point:
                selected_individuals.add(individual)
                break

    return list(selected_individuals)


# NOTE: For tests
# def roulette_with_removal(population: list[tuple[Any, float]], max_individuals: int) -> list[Any]:
#     """
#     Selects individuals based on the roulette wheel method (fitness-proportionate).

#     Each individual has a chance of being selected proportional to its fitness.

#     Parameters:
#         population (list[tuple[Any, float]]): List of tuples (individual, fitness).
#         max_individuals (int): Number of individuals to select.

#     Returns:
#         list[Any]: List of selected individuals.

#     Raises:
#         ValueError: If the total fitness is zero, making it impossible to calculate probabilities.
#     """
#     pop = population.copy()
#     selected_individuals = []

#     while len(selected_individuals) < min(len(pop), max_individuals):
#         total_fitness = sum(fitness for _, fitness in pop)

#         if total_fitness == 0:
#             print(f"Total fitness is zero for population {population}. cannot perform roulette selection.", flush=True)
#             break

#         selection_probabilities = [fitness / total_fitness for _, fitness in pop]

#         selection_point = random.random()
#         running_sum_of_probabilities = 0
#         for idx, (individual, selection_probability) in enumerate(zip(population, selection_probabilities)):
#             running_sum_of_probabilities += selection_probability
#             if running_sum_of_probabilities >= selection_point:
#                 selected_individuals.append(individual)
#                 del pop[idx]
#                 break

#     return selected_individuals
