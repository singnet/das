import unittest

import pytest

from hyperon_das.tokenizers.elements import (
    AndOperator,
    ElementBuilder,
    Link,
    Node,
    NotOperator,
    OrOperator,
    Variable,
)


class TestElements(unittest.TestCase):
    def test_node_to_tokens(self):
        node = Node(type="Symbol", name="TestNode")
        self.assertEqual(node.to_tokens(), ["NODE", "Symbol", "TestNode"])

    def test_node_from_tokens(self):
        tokens = ["NODE", "Symbol", "TestNode"]
        cursor, node = Node.from_tokens(tokens)
        self.assertIsInstance(node, Node)
        self.assertEqual(cursor, len(tokens))
        self.assertEqual(node.type, "Symbol")
        self.assertEqual(node.name, "TestNode")

    def test_variable_to_tokens(self):
        variable = Variable(name="TestVariable")
        self.assertEqual(variable.to_tokens(), ["VARIABLE", "TestVariable"])

    def test_variable_from_tokens(self):
        tokens = ["VARIABLE", "TestVariable"]
        cursor, variable = Variable.from_tokens(tokens)
        self.assertIsInstance(variable, Variable)
        self.assertEqual(cursor, len(tokens))
        self.assertEqual(variable.name, "TestVariable")

    def test_link_to_tokens(self):
        node = Node(type="Symbol", name="TestNode")
        variable = Variable(name="V1")
        link = Link(type="Expression", targets=[node, variable])
        self.assertEqual(
            link.to_tokens(),
            "LINK_TEMPLATE Expression 2 NODE Symbol TestNode VARIABLE V1".split(),
        )

    def test_link_from_tokens(self):
        tokens = "LINK_TEMPLATE Expression 2 NODE Symbol TestNode VARIABLE V1".split()
        cursor, link = Link.from_tokens(tokens)
        self.assertIsInstance(link, Link)
        self.assertEqual(cursor, len(tokens))
        self.assertEqual(link.type, "Expression")
        self.assertTrue(link.is_template)
        self.assertEqual(len(link.targets), 2)
        self.assertIsInstance(link.targets[0], Node)
        self.assertIsInstance(link.targets[1], Variable)

    def test_wrong_items_count(self):
        tokens = [
            "LINK_TEMPLATE",
            "Expression",
            "3",  # Wrong target count - should be 2
            *["NODE", "Symbol", "TestNode", "VARIABLE", "TestVariable"],
        ]
        with pytest.raises(IndexError, match="list index out of range"):
            Link.from_tokens(tokens)
        tokens = [
            "OR",
            "4",  # Wrong operand count - should be 3
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode1 VARIABLE V1".split()),
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode2 VARIABLE V2".split()),
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode3 VARIABLE V3".split()),
        ]
        with pytest.raises(IndexError, match="list index out of range"):
            OrOperator.from_tokens(tokens)

    def test_or_operator_to_tokens(self):
        node1 = Node(type="Symbol", name="TestNode1")
        node2 = Node(type="Symbol", name="TestNode2")
        variable = Variable(name="V1")
        link1 = Link(type="Expression", targets=[node1, variable])
        link2 = Link(type="Expression", targets=[node2, variable])
        or_operator = OrOperator(operands=[link1, link2])
        self.assertEqual(
            or_operator.to_tokens(),
            [
                "OR",
                "2",
                *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode1 VARIABLE V1".split()),
                *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode2 VARIABLE V1".split()),
            ],
        )

    def test_or_operator_from_tokens(self):
        tokens = [
            "OR",
            "2",
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode1 VARIABLE V1".split()),
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode2 VARIABLE V1".split()),
        ]
        cursor, or_operator = OrOperator.from_tokens(tokens)
        self.assertEqual(cursor, len(tokens))
        self.assertEqual(len(or_operator.operands), 2)
        self.assertIsInstance(or_operator.operands[0], Link)
        self.assertIsInstance(or_operator.operands[1], Link)

    def test_and_operator_to_tokens(self):
        node1 = Node(type="Symbol", name="TestNode1")
        node2 = Node(type="Symbol", name="TestNode2")
        variable = Variable(name="V1")
        link1 = Link(type="Expression", targets=[node1, variable])
        link2 = Link(type="Expression", targets=[node2, variable])
        and_operator = AndOperator(operands=[link1, link2])
        self.assertEqual(
            and_operator.to_tokens(),
            [
                "AND",
                "2",
                *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode1 VARIABLE V1".split()),
                *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode2 VARIABLE V1".split()),
            ],
        )

    def test_and_operator_from_tokens(self):
        tokens = [
            "AND",
            "2",
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode1 VARIABLE V1".split()),
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode2 VARIABLE V1".split()),
        ]
        cursor, and_operator = AndOperator.from_tokens(tokens)
        self.assertEqual(cursor, len(tokens))
        self.assertEqual(len(and_operator.operands), 2)
        self.assertIsInstance(and_operator.operands[0], Link)
        self.assertIsInstance(and_operator.operands[1], Link)

    def test_not_operator_to_tokens(self):
        node = Node(type="Symbol", name="TestNode")
        variable = Variable(name="V1")
        link = Link(type="Expression", targets=[node, variable])
        not_operator = NotOperator(operand=link)
        self.assertEqual(
            not_operator.to_tokens(),
            [
                "NOT",
                *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode VARIABLE V1".split()),
            ],
        )

    def test_not_operator_from_tokens(self):
        tokens = [
            "NOT",
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode VARIABLE V1".split()),
        ]
        cursor, not_operator = NotOperator.from_tokens(tokens)
        self.assertEqual(cursor, len(tokens))
        self.assertIsInstance(not_operator.operand, Link)

    def test_element_builder_from_tokens(self):
        tokens = ["NODE", "Symbol", "TestNode"]
        cursor, element = ElementBuilder.from_tokens(tokens)
        self.assertEqual(cursor, len(tokens))
        self.assertIsInstance(element, Node)
        self.assertEqual(element.type, "Symbol")
        self.assertEqual(element.name, "TestNode")

    def test_invalid_node_from_tokens(self):
        tokens = ["INVALID", "Symbol", "TestNode"]
        with pytest.raises(ValueError, match="Unsupported sequence of tokens:"):
            Node.from_tokens(tokens)

    def test_invalid_variable_from_tokens(self):
        tokens = ["INVALID", "TestVariable"]
        with pytest.raises(ValueError, match="Unsupported sequence of tokens:"):
            Variable.from_tokens(tokens)

    def test_invalid_link_from_tokens(self):
        tokens = [
            "LINK_TEMPLATE",
            "Expression",
            "1",  # Invalid target count (less than 2)
            *["NODE", "Symbol", "TestNode"],
        ]
        with pytest.raises(ValueError, match="Link requires at least two targets"):
            Link.from_tokens(tokens)

    def test_invalid_or_operator_from_tokens(self):
        tokens = [
            "OR",
            "1",  # Invalid operand count (less than 2)
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode VARIABLE V1".split()),
        ]
        with pytest.raises(ValueError, match="OR operator requires at least two operands"):
            OrOperator.from_tokens(tokens)

    def test_invalid_and_operator_from_tokens(self):
        tokens = [
            "AND",
            "1",  # Invalid operand count (less than 2)
            *("LINK_TEMPLATE Expression 2 NODE Symbol TestNode VARIABLE V1".split()),
        ]
        with pytest.raises(ValueError, match="AND operator requires at least two operands"):
            AndOperator.from_tokens(tokens)

    def test_invalid_not_operator_from_tokens(self):
        tokens = [
            "NOT",
            "INVALID",
            "Expression",
            "2",  # Invalid operand type
            *["NODE", "Symbol", "TestNode", "VARIABLE", "TestVariable"],
        ]
        with pytest.raises(ValueError, match="Unsupported sequence of tokens:"):
            NotOperator.from_tokens(tokens)


if __name__ == "__main__":
    unittest.main()
