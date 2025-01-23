from typing import List

from hyperon_das_atomdb.database import NodeT

from hyperon_das.utils import QueryAnswer


class Context:
    CONTEXT_NODE_TYPE = "Context"

    def __init__(self, context_node: NodeT, query_answers: List[List[QueryAnswer]]):
        self.name = context_node.name
        self.handle = context_node.id
        self.query_answers = query_answers
