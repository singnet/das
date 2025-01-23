import unittest

import pytest

from hyperon_das.tokenizers.dict_query_tokenizer import DictQueryTokenizer


class TestDictQueryTokenizer(unittest.TestCase):
    def test_tokenize_link(self):
        query = {
            "atom_type": "link",
            "type": "Expression",
            "targets": [
                {"atom_type": "node", "type": "Symbol", "name": "Similarity"},
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "Concept"},
                        {"atom_type": "node", "type": "Symbol", "name": '"human"'},
                    ],
                },
                {"atom_type": "variable", "name": "v1"},
            ],
        }
        expected_tokens = (
            'LINK_TEMPLATE Expression 3 NODE Symbol Similarity LINK Expression 2 '
            'NODE Symbol Concept NODE Symbol "human" VARIABLE v1'
        )
        assert DictQueryTokenizer.tokenize(query) == expected_tokens

    def test_untokenize_link(self):
        tokens = (
            'LINK_TEMPLATE Expression 3 NODE Symbol Similarity LINK Expression 2 '
            'NODE Symbol Concept NODE Symbol "human" VARIABLE v1'
        )
        expected_query = {
            "atom_type": "link",
            "type": "Expression",
            "targets": [
                {"atom_type": "node", "type": "Symbol", "name": "Similarity"},
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "Concept"},
                        {"atom_type": "node", "type": "Symbol", "name": '"human"'},
                    ],
                },
                {"atom_type": "variable", "name": "v1"},
            ],
        }
        assert DictQueryTokenizer.untokenize(tokens) == expected_query

    def test_tokenize_invalid_query(self):
        query = {"atom_type": "unknown", "name": "InvalidQuery"}
        with pytest.raises(
            ValueError, match="Unsupported query, it should start with a link or an operator:"
        ):
            DictQueryTokenizer.tokenize(query)

    def test_untokenize_invalid_tokens(self):
        # Unknown token
        tokens = "UNKNOWN InvalidQuery"
        with pytest.raises(ValueError, match="Unsupported sequence of tokens:"):
            DictQueryTokenizer.untokenize(tokens)
        # LINK cannot be the first token
        tokens = "LINK Expression 2 NODE Symbol Similarity NODE Symbol Anything"
        with pytest.raises(ValueError, match="Unsupported sequence of tokens:"):
            DictQueryTokenizer.untokenize(tokens)

    def test_untokenize_wrong_elements_count(self):
        tokens = (
            "LINK_TEMPLATE Expression "
            "3"  # Wrong targets count - should be 2
            " NODE Symbol TestNode VARIABLE TestVariable"
        )
        with pytest.raises(IndexError, match="list index out of range"):
            DictQueryTokenizer.untokenize(tokens)

        tokens = (
            "OR "
            "2"  # Wrong operands count - should be 3
            " LINK_TEMPLATE Expression 2 NODE Symbol N1 VARIABLE V1"
            " LINK_TEMPLATE Expression 2 NODE Symbol N2 VARIABLE V2"
            " LINK_TEMPLATE Expression 2 NODE Symbol N3 VARIABLE V3"
        )
        with pytest.raises(ValueError, match="Wrong elements count"):
            DictQueryTokenizer.untokenize(tokens)

    def test_tokenize_invalid_start_node(self):
        query = {"atom_type": "node", "type": "Symbol", "name": "TestNode"}
        with pytest.raises(
            ValueError, match="Unsupported query, it should start with a link or an operator:"
        ):
            DictQueryTokenizer.tokenize(query)

    def test_tokenize_invalid_start_variable(self):
        query = {"atom_type": "variable", "name": "TestVariable"}
        with pytest.raises(
            ValueError, match="Unsupported query, it should start with a link or an operator:"
        ):
            DictQueryTokenizer.tokenize(query)

    def test_tokenize_and_operator(self):
        query = {
            "and": [
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode1"},
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode2"},
                    ],
                },
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode2"},
                        {"atom_type": "variable", "name": "TestVariable2"},
                    ],
                },
            ]
        }
        expected_tokens = (
            "AND 2 "
            "LINK Expression 2 NODE Symbol TestNode1 NODE Symbol TestNode2 "
            "LINK_TEMPLATE Expression 2 NODE Symbol TestNode2 VARIABLE TestVariable2"
        )
        assert DictQueryTokenizer.tokenize(query) == expected_tokens

    def test_untokenize_and_operator(self):
        tokens = (
            "AND 2 "
            "LINK Expression 2 NODE Symbol TestNode1 NODE Symbol TestNode2 "
            "LINK_TEMPLATE Expression 2 NODE Symbol TestNode2 VARIABLE TestVariable2"
        )
        expected_query = {
            "and": [
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode1"},
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode2"},
                    ],
                },
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode2"},
                        {"atom_type": "variable", "name": "TestVariable2"},
                    ],
                },
            ]
        }
        assert DictQueryTokenizer.untokenize(tokens) == expected_query

    def test_tokenize_or_operator(self):
        query = {
            "or": [
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode1"},
                        {"atom_type": "variable", "name": "TestVariable1"},
                    ],
                },
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode2"},
                        {"atom_type": "variable", "name": "TestVariable2"},
                    ],
                },
            ]
        }
        expected_tokens = (
            "OR 2 "
            "LINK_TEMPLATE Expression 2 NODE Symbol TestNode1 VARIABLE TestVariable1 "
            "LINK_TEMPLATE Expression 2 NODE Symbol TestNode2 VARIABLE TestVariable2"
        )
        assert DictQueryTokenizer.tokenize(query) == expected_tokens

    def test_untokenize_or_operator(self):
        tokens = (
            "OR 2 "
            "LINK_TEMPLATE Expression 2 NODE Symbol TestNode1 VARIABLE TestVariable1 "
            "LINK_TEMPLATE Expression 2 NODE Symbol TestNode2 VARIABLE TestVariable2"
        )
        expected_query = {
            "or": [
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode1"},
                        {"atom_type": "variable", "name": "TestVariable1"},
                    ],
                },
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "node", "type": "Symbol", "name": "TestNode2"},
                        {"atom_type": "variable", "name": "TestVariable2"},
                    ],
                },
            ]
        }
        assert DictQueryTokenizer.untokenize(tokens) == expected_query

    def test_tokenize_not_operator(self):
        query = {
            "not": {
                "atom_type": "link",
                "type": "Expression",
                "targets": [
                    {"atom_type": "node", "type": "Symbol", "name": "TestNode"},
                    {"atom_type": "variable", "name": "TestVariable"},
                ],
            }
        }
        expected_tokens = (
            "NOT LINK_TEMPLATE Expression 2 NODE Symbol TestNode VARIABLE TestVariable"
        )
        assert DictQueryTokenizer.tokenize(query) == expected_tokens

    def test_untokenize_not_operator(self):
        tokens = "NOT LINK_TEMPLATE Expression 2 NODE Symbol TestNode VARIABLE TestVariable"
        expected_query = {
            "not": {
                "atom_type": "link",
                "type": "Expression",
                "targets": [
                    {"atom_type": "node", "type": "Symbol", "name": "TestNode"},
                    {"atom_type": "variable", "name": "TestVariable"},
                ],
            }
        }
        assert DictQueryTokenizer.untokenize(tokens) == expected_query


if __name__ == "__main__":
    unittest.main()
