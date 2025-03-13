from unittest import TestCase

from hyperon_das_query_engine import (
    DASNode,
    QueryAnswer,
    HandlesAnswer,
    RemoteIterator,
)


class TestDASNode(TestCase):
    def test_das_node_binding(self) -> None:
        """
        Test if all the attributes are present on the binding of DASNode
        """

        #inherited attributes
        self.assertTrue(hasattr(DASNode, "add_peer"))
        self.assertTrue(hasattr(DASNode, "broadcast"))
        self.assertTrue(hasattr(DASNode, "cast_leadership_vote"))
        self.assertTrue(hasattr(DASNode, "has_leader"))
        self.assertTrue(hasattr(DASNode, "is_leader"))
        self.assertTrue(hasattr(DASNode, "join_network"))
        self.assertTrue(hasattr(DASNode, "message_factory"))
        self.assertTrue(hasattr(DASNode, "node_id"))
        self.assertTrue(hasattr(DASNode, "node_joined_network"))
        self.assertTrue(hasattr(DASNode, "node_joined_network"))
        self.assertTrue(hasattr(DASNode, "send"))

        #own attributes
        self.assertTrue(hasattr(DASNode, "PATTERN_MATCHING_QUERY"))
        self.assertTrue(hasattr(DASNode, "COUNTING_QUERY"))
        self.assertTrue(hasattr(DASNode, "pattern_matcher_query"))
        self.assertTrue(hasattr(DASNode, "count_query"))
        self.assertTrue(hasattr(DASNode, "next_query_id"))

    def test_query_answer_bindings(self):
        self.assertTrue(hasattr(QueryAnswer, "tokenize"))
        self.assertTrue(hasattr(QueryAnswer, "untokenize"))
        self.assertTrue(hasattr(QueryAnswer, "to_string"))

    def test_handles_answer_inherits_query_answer(self):
        self.assertTrue(issubclass(HandlesAnswer, QueryAnswer))

    def test_handles_answer_bindings(self):
        # self.assertTrue(hasattr(HandlesAnswer, "handles"))
        self.assertTrue(hasattr(HandlesAnswer, "handles_size"))
        self.assertTrue(hasattr(HandlesAnswer, "importance"))
        self.assertTrue(hasattr(HandlesAnswer, "add_handle"))
        self.assertTrue(hasattr(HandlesAnswer, "merge"))
        self.assertTrue(hasattr(HandlesAnswer, "copy"))

        # inherited attr
        self.assertTrue(hasattr(HandlesAnswer, "tokenize"))
        self.assertTrue(hasattr(HandlesAnswer, "untokenize"))
        self.assertTrue(hasattr(HandlesAnswer, "to_string"))

    def test_remote_iterator_bindings(self):
        self.assertTrue(hasattr(RemoteIterator, "is_terminal"))
        self.assertTrue(hasattr(RemoteIterator, "graceful_shutdown"))
        self.assertTrue(hasattr(RemoteIterator, "setup_buffers"))
        self.assertTrue(hasattr(RemoteIterator, "finished"))
        self.assertTrue(hasattr(RemoteIterator, "pop"))

    # def test_das_node(self) -> None:
    #     """
    #     Test das_node server and client constructors
    #     """
    #     self.server_id: str = "localhost:35700"
    #     self.client1_id: str = "localhost:35701"
    #     self.client2_id: str = "localhost:35702"
    #
    #     self.server = DASNode(node_id=self.server_id)
    #     self.client1 = DASNode(node_id=self.client1_id, server_id=self.server_id)
    #     self.client2 = DASNode(node_id=self.client2_id, server_id=self.server_id)
    #
    #     # Test id assignment
    #     self.assertEqual(self.server.node_id(), self.server_id)
    #     self.assertEqual(self.client1.node_id(), self.client1_id)
    #     self.assertEqual(self.client2.node_id(), self.client2_id)
    #
    #     # Server should be leader
    #     self.assertTrue(self.server.has_leader())
    #     self.assertTrue(self.client1.has_leader())
    #     self.assertTrue(self.client2.has_leader())
    #
    #     self.assertTrue(self.server.is_leader())
    #     self.assertFalse(self.client1.is_leader())
    #     self.assertFalse(self.client2.is_leader())
    #
    #     self.assertEqual(self.server.leader_id(), self.server_id)
    #     self.assertEqual(self.client1.leader_id(), self.server_id)
    #     self.assertEqual(self.client2.leader_id(), self.server_id)
    #
    #     # Test id assignment
    #     self.assertEqual(self.server.node_id(), self.server_id)
    #     self.assertEqual(self.client1.node_id(), self.client1_id)
    #     self.assertEqual(self.client2.node_id(), self.client2_id)
    #
