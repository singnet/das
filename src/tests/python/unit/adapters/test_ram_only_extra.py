import pytest

from hyperon_das_atomdb.adapters.ram_only import InMemoryDB
from tests.python.unit.fixtures import in_memory_db  # noqa: F401
from tests.python.unit.test_database_public_methods import check_handle


@pytest.mark.skip("testing protected members - must be moved to the C++ implementation.")
class TestRamOnlyExtra:
    def test__build_atom_type_key_hash(self, in_memory_db):  # noqa: F811
        db: InMemoryDB = in_memory_db
        hash = db._build_atom_type_key_hash("A")
        assert check_handle(hash)
        assert hash == "2c832bdcd9d74bf961205676d861540a"

    def test__delete_atom_type(self, in_memory_db):  # noqa: F811
        db: InMemoryDB = in_memory_db
        node = db.add_node({"name": "A", "type": "A"})
        assert len(db.all_named_types) == 1
        assert node["named_type"] in db.all_named_types
        db._delete_atom_type("A")
        assert len(db.all_named_types) == 0
        assert node["named_type"] not in db.all_named_types

    def test__update_atom_indexes(self, in_memory_db):  # noqa: F811
        db: InMemoryDB = in_memory_db
        node = db.add_node({"name": "A", "type": "A"})
        db._update_atom_indexes([node])
        assert len(db.all_named_types) == 1
