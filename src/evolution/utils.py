import ast
import functools
import os
import sys
import time

from collections import namedtuple
from typing import Any


from dataclasses import dataclass


@dataclass
class Parameters:
    number_nodes: int = None
    population_size: int = None
    max_generations: int = None
    qtd_selected_for_attention_update: int = None
    max_query_answers: int = None
    selection_method: str = None
    fitness_function: callable = None
    query_agent_node_id: int = None
    query_agent_server_id: int = None
    attention_broker_server_id: str = None
    context: str = None
    mongo_hostname: str = None
    mongo_port: int = None
    mongo_username: str = None
    mongo_password: str = None
    redis_hostname: str = None
    redis_port: int = None
    redis_cluster: bool = None
    redis_ssl: bool = None


# NOTE: Used for development only
class SuppressCppOutput:
    """Redirects C++ stdout to suppress prints from std::cout."""

    def __enter__(self):
        self._original_stdout_fd = sys.stdout.fileno()
        self._saved_stdout_fd = os.dup(self._original_stdout_fd)
        self._devnull = os.open(os.devnull, os.O_WRONLY)
        os.dup2(self._devnull, self._original_stdout_fd)

    def __exit__(self, exc_type, exc_value, traceback):
        os.dup2(self._saved_stdout_fd, self._original_stdout_fd)
        os.close(self._saved_stdout_fd)
        os.close(self._devnull)


def parse_file(path) -> dict[str, Any]:
    config = {}
    with open(path, mode='r') as f:
        for line in f:
            if not (line := line.split('#')[0].strip()):
                continue

            parts = line.split('=')
            if len(parts) != 2:
                raise ValueError(f"Line is not in key=value format: {line}")

            key = parts[0].strip()
            value = parts[1].strip()

            try:
                value = ast.literal_eval(value)
            except (ValueError, SyntaxError):
                pass

            config[key] = value
    return config


def profile(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.perf_counter()
        result = func(*args, **kwargs)
        end_time = time.perf_counter()
        elapsed_time = end_time - start_time
        print(f"{func.__name__} executed in {elapsed_time:.6f} seconds")
        return result
    return wrapper
