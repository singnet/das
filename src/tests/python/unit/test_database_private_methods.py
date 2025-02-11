import pytest

from hyperon_das_atomdb.database import AtomDB, LinkT, NodeT
from hyperon_das_atomdb.exceptions import AddLinkException, AddNodeException, AtomDoesNotExist
from tests.python.helpers import add_link, add_node, check_handle

from .fixtures import in_memory_db, redis_mongo_db  # noqa: F401


class TestDatabasePrivateMethods:
    @pytest.mark.parametrize("database", ["redis_mongo_db", "in_memory_db"])
    def test__get_atom(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        link_a = add_link(db, "Aa", [node_a], database)
        node = db._get_atom(node_a.handle)
        link = db._get_atom(link_a.handle)
        assert node, link

    @pytest.mark.parametrize("database", ["redis_mongo_db", "in_memory_db"])
    def test__get_atom_none(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        node = db._get_atom("handle")
        assert node is None

    @pytest.mark.parametrize(
        "database,kwlist",
        [
            ("redis_mongo_db", ["targets_document", "deep_representation"]),
            ("in_memory_db", ["targets_document", "deep_representation"]),
        ],
    )
    def test__reformat_document(self, database, kwlist, request):
        db: AtomDB = request.getfixturevalue(database)
        link = db.add_link(
            LinkT(
                type="Relation",
                targets=[
                    NodeT(name="A", type="Test"),
                    NodeT(name="B", type="Test"),
                ],
            ),
        )
        if database != "in_memory_db":
            db.commit()
        for kw in kwlist:
            answer = db._reformat_document(link, **{kw: True})
            assert answer.targets_documents is not None
            assert len(answer.targets) == 2
            assert len(answer.targets_documents) == 2
            assert answer.named_type == "Relation"
            assert all(
                isinstance(t, NodeT) for t in answer.targets_documents
            ), answer.targets_documents

    @pytest.mark.parametrize(
        "database,kwlist",
        [
            ("redis_mongo_db", ["targets_document", "deep_representation"]),
            ("in_memory_db", ["targets_document", "deep_representation"]),
        ],
    )
    def test__reformat_document_exceptions(self, database, kwlist, request):
        db: AtomDB = request.getfixturevalue(database)
        link = LinkT(
            _id="dummy",
            handle="dummy",
            composite_type_hash="dummy",
            composite_type=["dummy"],
            named_type="dummy",
            named_type_hash="dummy",
            is_toplevel=True,
            targets=["dummy"],
        )
        for kw in kwlist:
            with pytest.raises(AtomDoesNotExist, match="Nonexistent atom"):
                db._reformat_document(link, **{kw: True})

    @pytest.mark.parametrize(
        "database,expected_fields, expected_handle",
        [
            (
                "redis_mongo_db",
                ["handle", "_id", "composite_type_hash", "name", "named_type"],
                "180fed764dbd593f1ea45b63b13d7e69",
            ),
            (
                "in_memory_db",
                ["handle", "_id", "composite_type_hash", "name", "named_type"],
                "180fed764dbd593f1ea45b63b13d7e69",
            ),
        ],
    )
    def test__build_node(self, database, expected_fields, expected_handle, request):
        db: AtomDB = request.getfixturevalue(database)
        node = db._build_node(NodeT(type="Test", name="test"))
        assert node
        assert node.handle == expected_handle
        assert all([k in node.to_dict() for k in expected_fields])
        assert isinstance(node, NodeT)
        assert check_handle(node.handle)

        # Test exception
        with pytest.raises(AddNodeException, match="'type' and 'name' are required."):
            db._build_node(NodeT(type="", name=""))

    @pytest.mark.parametrize(
        "database,expected_fields, expected_handle,is_toplevel",
        [
            (
                "redis_mongo_db",
                [
                    "handle",
                    "_id",
                    "composite_type_hash",
                    "named_type_hash",
                    "named_type",
                    "is_toplevel",
                    "targets",
                ],
                "180fed764dbd593f1ea45b63b13d7e69",
                True,
            ),
            (
                "redis_mongo_db",
                [
                    "handle",
                    "_id",
                    "composite_type_hash",
                    "named_type_hash",
                    "named_type",
                    "is_toplevel",
                    "targets",
                ],
                "180fed764dbd593f1ea45b63b13d7e69",
                False,
            ),
            (
                "in_memory_db",
                [
                    "handle",
                    "_id",
                    "composite_type_hash",
                    "named_type_hash",
                    "named_type",
                    "is_toplevel",
                    "targets",
                ],
                "180fed764dbd593f1ea45b63b13d7e69",
                True,
            ),
            (
                "in_memory_db",
                [
                    "handle",
                    "_id",
                    "composite_type_hash",
                    "named_type_hash",
                    "named_type",
                    "is_toplevel",
                    "targets",
                ],
                "180fed764dbd593f1ea45b63b13d7e69",
                False,
            ),
        ],
    )
    def test__build_link(self, database, expected_fields, expected_handle, is_toplevel, request):
        db: AtomDB = request.getfixturevalue(database)
        link = db._build_link(
            LinkT(
                type="Test",
                targets=[
                    NodeT(type="Test", name="test"),
                ],
            ),
            is_toplevel,
        )
        assert expected_handle in link.targets
        assert all([k in link.to_dict() for k in expected_fields])
        assert link.is_toplevel == is_toplevel
        assert check_handle(link.handle)
        assert isinstance(link, LinkT)
        assert isinstance(link.targets, list)

    @pytest.mark.parametrize("database", ["redis_mongo_db", "in_memory_db"])
    def test__build_link_exceptions(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(AddLinkException, match="'type' and 'targets' are required."):
            db._build_link(LinkT(type="Test", targets=[]))
        with pytest.raises(AddLinkException, match="'type' and 'targets' are required."):
            db._build_link(LinkT(type="", targets=[NodeT(type="Test", name="test")]))
