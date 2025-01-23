from typing import Any, Callable, Type, TypeAlias

from hyperon_das.tokenizers.elements import (
    TOKENS_DELIMITER,
    AndOperator,
    ElementBuilder,
    Link,
    Node,
    NotOperator,
    OrOperator,
    Variable,
)


class DictQueryTokenizer:
    """
    A class for tokenizing dictionary queries.


    Example usage of DictQueryTokenizer:

    Sample query dictionary:
        >>> sample_query = {
        >>>     "atom_type": "link",
        >>>     "type": "Expression",
        >>>     "targets": [
        >>>         {"atom_type": "node", "type": "Symbol", "name": "Similarity"},
        >>>         {
        >>>             "atom_type": "link",
        >>>             "type": "Expression",
        >>>             "targets": [
        >>>                 {"atom_type": "node", "type": "Symbol", "name": "Concept"},
        >>>                 {"atom_type": "node", "type": "Symbol", "name": '"human"'},
        >>>             ],
        >>>         },
        >>>         {"atom_type": "variable", "name": "v1"},
        >>>     ],
        >>> }

    Tokenizing the sample query:
        >>> print(DictQueryTokenizer.tokenize(sample_query))
        Output:
        LINK_TEMPLATE Expression 3 NODE Symbol Similarity LINK Expression 2 NODE Symbol Concept NODE Symbol "human" VARIABLE v1

    Sample tokenized string:
        >>> sample_tokens = '''
        >>>     LINK_TEMPLATE Expression 3
        >>>         NODE Symbol Similarity
        >>>         LINK Expression 2
        >>>             NODE Symbol Concept
        >>>             NODE Symbol "human"
        >>>         VARIABLE v1
        >>> '''

    Untokenizing the sample tokens:
        >>> print(DictQueryTokenizer.untokenize(sample_tokens))
        Output:
        {
          'atom_type': 'link',
          'type': 'Expression',
          'targets': [
            {'atom_type': 'node', 'type': 'Symbol', 'name': 'Similarity'},
            {
              'atom_type': 'link',
              'type': 'Expression',
              'targets': [
                {'atom_type': 'node', 'type': 'Symbol', 'name': 'Concept'},
                {'atom_type': 'node', 'type': 'Symbol', 'name': '"human"'}
              ]
            },
            {'atom_type': 'variable', 'name': 'v1'}
          ]
        }
    """

    Query: TypeAlias = dict[str, Any]
    """type alias for a dictionary representing a query."""

    ToQueryMappingKey: TypeAlias = Type[
        Node | Variable | Link | OrOperator | AndOperator | NotOperator
    ]
    ToQueryMappingValue: TypeAlias = Callable[
        [Node | Variable | Link | OrOperator | AndOperator | NotOperator],
        Query,
    ]
    to_query_mapping: dict[ToQueryMappingKey, ToQueryMappingValue] = {
        # A mapping of types to functions that convert instances of those types to query dictionaries.
        Node: lambda node: {
            "atom_type": "node",
            "type": node.type,
            "name": node.name,
        },
        Variable: lambda variable: {
            "atom_type": "variable",
            "name": variable.name,
        },
        Link: lambda link: {
            "atom_type": "link",
            "type": link.type,
            "targets": [
                DictQueryTokenizer.to_query_mapping[type(target)](target) for target in link.targets
            ],
        },
        OrOperator: lambda operator: {
            "or": [
                DictQueryTokenizer.to_query_mapping[type(operand)](operand)
                for operand in operator.operands
            ],
        },
        AndOperator: lambda operator: {
            "and": [
                DictQueryTokenizer.to_query_mapping[type(operand)](operand)
                for operand in operator.operands
            ],
        },
        NotOperator: lambda operator: {
            "not": DictQueryTokenizer.to_query_mapping[type(operator.operand)](operator.operand),
        },
    }

    @staticmethod
    def tokenize(query: Query) -> str:
        """
        Convert a query dictionary into a tokenized string.

        Args:
            query (Query): The query dictionary to tokenize.

        Returns:
            str: A tokenized string representation of the query.

        Raises:
            ValueError: If the query cannot be tokenized.
        """

        def _tokenize(
            _query: DictQueryTokenizer.Query, _parent: Link | None = None
        ) -> Link | Node | Variable | OrOperator | AndOperator:
            match _query:
                case {"or": list()}:
                    return OrOperator(operands=[_tokenize(o) for o in _query["or"]])
                case {"and": list()}:
                    return AndOperator(operands=[_tokenize(o) for o in _query["and"]])
                case {"not": dict()}:
                    return NotOperator(_tokenize(_query["not"]))
                case {"atom_type": "link", "type": str(), "targets": list()}:
                    link = Link(_query["type"])
                    link.targets += [_tokenize(t, _parent=link) for t in _query["targets"]]
                    return link
                case {"atom_type": "node", "type": str(), "name": str()}:
                    return Node(_query["type"], _query["name"])
                case {"atom_type": "variable", "name": str()}:
                    _parent.is_template = True
                    return Variable(_query["name"])
                case _:
                    raise ValueError(f"Unsupported query: {_query}")

        match query:
            case {"or": list()} | {"and": list()} | {"not": dict()} | {
                "atom_type": "link",
                "type": str(),
                "targets": list(),
            }:
                return TOKENS_DELIMITER.join(_tokenize(query).to_tokens())
            case _:
                raise ValueError(
                    f"Unsupported query, it should start with a link or an operator: {query=}"
                )

    @staticmethod
    def untokenize(query_tokens: str) -> Query:
        """
        Convert a tokenized string back into a query dictionary.

        Args:
            query_tokens (str): The tokenized string to untokenize.

        Returns:
            Query: A dictionary representation of the query.

        Raises:
            ValueError: If the tokens cannot be untokenized into a valid query.
        """
        cursor = 0
        tokens = query_tokens.split()
        if len(tokens) > 0 and tokens[0] in ("AND", "OR", "NOT", "LINK_TEMPLATE"):
            cursor, element = ElementBuilder.from_tokens(tokens, cursor)
            if cursor != len(tokens):
                raise ValueError("Wrong elements count")
            if to_query_callback := DictQueryTokenizer.to_query_mapping.get(type(element)):
                return to_query_callback(element)
        raise ValueError(f"Unsupported sequence of tokens: {tokens[cursor:]}")
