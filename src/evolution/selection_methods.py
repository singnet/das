"""
Module for selecting individuals based on fitness.

This module implements two selection methods:
  - best_fitness: Selects individuals with the highest fitness values.
  - fitness_proportionate: Selects individuals proportionally to their fitness (roulette wheel).

It also provides a function to retrieve the appropriate selection function based on a specified method.
"""

import enum
import random
from typing import Any


class SelectionMethodType(str, enum.Enum):
    BEST_FITNESS = "best_fitness"
    ROULETTE = "roulette"


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
            individual, _ = evaluated_sorted.pop(0)
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
    total_fitness = sum([fitness for _, fitness in population])

    if total_fitness == 0:
        raise ValueError("Total fitness is zero; cannot perform roulette selection.")

    selection_probabilities = [fitness / total_fitness for _, fitness in population]
    selected_individuals = []

    for _ in range(max_individuals):
        selection_point = random.random()
        running_sum_of_probabilities = 0
        for (individual, _), selection_probability in zip(population, selection_probabilities):
            running_sum_of_probabilities += selection_probability
            if running_sum_of_probabilities >= selection_point:
                selected_individuals.append(individual)
                break

    return selected_individuals
