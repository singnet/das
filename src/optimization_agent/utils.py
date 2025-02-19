from typing import Any
from collections import namedtuple


def parse_file(path) -> list[Any]:
    ret = []
    with open(path, mode='r') as f:
        lines = f.readlines()
        for line in lines:
            value = line.strip().replace(" ", "").split("=")[-1]
            ret.append(eval(value))
    return ret


Parameters = namedtuple(
    typename='Parameters',
    field_names=[
        'population_size',
        'max_generations',
        'qtd_selected_for_attention_update',
        'max_query_answers',
        'selection_method',
        'fitness_function',
        'node_id',
        'das_node_server_id',
        'attention_broker_server',
        'context',
        'mongo_hostname',
        'mongo_port',
        'mongo_username',
        'mongo_password',
        'redis_hostname',
        'redis_port',
        'redis_cluster',
        'redis_ssl'
    ]
)
