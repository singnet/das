from hyperon_das_atomdb.database import NodeT

from hyperon_das.context import Context


class TestContext:
    def test_creation(self):
        context_name = "blah"
        context_handle = "h"
        node = NodeT(type="Context", name=context_name)
        node.handle = node._id = context_handle
        context = Context(node, [])
        assert context.name == context_name
        assert context.handle == context_handle
