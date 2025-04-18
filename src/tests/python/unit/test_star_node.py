from unittest import TestCase
from time import sleep

from hyperon_das_node import StarNode


class TestStarNode(TestCase):
    def test_star_node_binding(self):
        self.assertTrue(hasattr(StarNode, "add_peer"))
        self.assertTrue(hasattr(StarNode, "broadcast"))
        self.assertTrue(hasattr(StarNode, "cast_leadership_vote"))
        self.assertTrue(hasattr(StarNode, "has_leader"))
        self.assertTrue(hasattr(StarNode, "is_leader"))
        self.assertTrue(hasattr(StarNode, "join_network"))
        self.assertTrue(hasattr(StarNode, "message_factory"))
        self.assertTrue(hasattr(StarNode, "node_id"))
        self.assertTrue(hasattr(StarNode, "node_joined_network"))
        self.assertTrue(hasattr(StarNode, "node_joined_network"))
        self.assertTrue(hasattr(StarNode, "send"))

    def test_star_node(self):
        self.server_id: str = "localhost:35700"
        self.client1_id: str = "localhost:35701"
        self.client2_id: str = "localhost:35702"

        self.server = StarNode(node_id=self.server_id)
        self.client1 = StarNode(node_id=self.client1_id, server_id=self.server_id)
        self.client2 = StarNode(node_id=self.client2_id, server_id=self.server_id)

        sleep(1)

        # Test id assignment
        self.assertEqual(self.server.node_id(), self.server_id)
        self.assertEqual(self.client1.node_id(), self.client1_id)
        self.assertEqual(self.client2.node_id(), self.client2_id)

        # Server should be leader
        self.assertTrue(self.server.has_leader())
        self.assertTrue(self.client1.has_leader())
        self.assertTrue(self.client2.has_leader())

        self.assertTrue(self.server.is_leader())
        self.assertFalse(self.client1.is_leader())
        self.assertFalse(self.client2.is_leader())

        self.assertEqual(self.server.leader_id(), self.server_id)
        self.assertEqual(self.client1.leader_id(), self.server_id)
        self.assertEqual(self.client2.leader_id(), self.server_id)

        # Test id assignment
        self.assertEqual(self.server.node_id(), self.server_id)
        self.assertEqual(self.client1.node_id(), self.client1_id)
        self.assertEqual(self.client2.node_id(), self.client2_id)
