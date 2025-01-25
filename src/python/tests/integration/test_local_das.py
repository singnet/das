# flake8: noqa F811

from hyperon_das_atomdb.database import LinkT, NodeT

from hyperon_das import DistributedAtomSpace
from tests.integration.helpers import (
    das_local_custom_fixture,
    das_local_fixture,
    das_remote_fixture_module,
    get_remote_das_port,
    mongo_port,
    redis_port,
    remote_das_host,
)
from tests.helpers import load_animals_base


class TestLocalDASRedisMongo:
    def test_queries(self, das_local_fixture: DistributedAtomSpace):
        das = das_local_fixture
        assert das.count_atoms() == {"atom_count": 0}
        load_animals_base(das)
        assert das.count_atoms() == {"atom_count": 0}
        das.commit_changes()
        assert das.count_atoms() == {"atom_count": 40}
        assert das.count_atoms({"precise": True}) == {
            "atom_count": 40,
            "node_count": 14,
            "link_count": 26,
        }

    def test_add_atom_persistence(self, das_local_fixture: DistributedAtomSpace):
        das = das_local_fixture
        assert das.count_atoms() == {"atom_count": 0}
        load_animals_base(das)
        assert das.count_atoms() == {"atom_count": 0}
        das.commit_changes()
        assert das.count_atoms() == {"atom_count": 40}
        assert das.count_atoms({"context": "remote"}) == {}

        das.add_node(NodeT(**{"type": "Concept", "name": "dog"}))
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "dog"}),
                        NodeT(**{"type": "Concept", "name": "mammal"}),
                    ],
                }
            )
        )
        das.commit_changes()
        assert das.count_atoms({"precise": True}) == {
            "atom_count": 42,
            "node_count": 15,
            "link_count": 27,
        }

        das2 = DistributedAtomSpace(
            query_engine="local",
            atomdb="redis_mongo",
            mongo_port=mongo_port,
            mongo_username="dbadmin",
            mongo_password="dassecret",
            redis_port=redis_port,
            redis_cluster=False,
            redis_ssl=False,
        )
        assert das2.count_atoms({"precise": True}) == {
            "atom_count": 42,
            "node_count": 15,
            "link_count": 27,
        }

    def test_fetch_atoms_from_remote_server(
        self, das_local_fixture: DistributedAtomSpace, das_remote_fixture_module
    ):
        das = das_local_fixture
        _ = das_remote_fixture_module
        assert das.count_atoms() == {"atom_count": 0}
        das.fetch(
            query={
                "atom_type": "link",
                "type": "Expression",
                "targets": [
                    {"atom_type": "node", "type": "Symbol", "name": "Inheritance"},
                    {"atom_type": "variable", "name": "v1"},
                    {"atom_type": "node", "type": "Symbol", "name": '"mammal"'},
                ],
            },
            host=remote_das_host,
            port=get_remote_das_port(),
        )
        assert das.count_atoms({"precise": True}) == {
            "atom_count": 10,
            "node_count": 6,
            "link_count": 4,
        }

    def test_fetch_atoms(self, das_local_custom_fixture):
        das = das_local_custom_fixture(
            query_engine='local',
            atomdb='redis_mongo',
            mongo_port=mongo_port,
            mongo_username='dbadmin',
            mongo_password='dassecret',
            redis_port=redis_port,
            redis_cluster=False,
            redis_ssl=False,
            system_parameters={"running_on_server": True},
        )
        assert das.count_atoms() == {"atom_count": 0}
        load_animals_base(das)
        das.commit_changes()
        assert das.count_atoms() == {"atom_count": 40}
        assert das.count_atoms({"precise": True}) == {
            "atom_count": 40,
            "node_count": 14,
            "link_count": 26,
        }
        documents = das.fetch(
            query={
                "atom_type": "link",
                "type": "Inheritance",
                "targets": [
                    {"atom_type": "variable", "name": "v1"},
                    {"atom_type": "node", "type": "Concept", "name": "mammal"},
                ],
            },
        )
        assert len(documents) == 9


class TestLocalDASRamOnly:
    def test_fetch_atoms_local_das_ram_only(self, das_remote_fixture_module):
        das = DistributedAtomSpace()
        _ = das_remote_fixture_module
        assert das.count_atoms() == {"atom_count": 0, "link_count": 0, "node_count": 0}
        das.fetch(
            query={
                "atom_type": "link",
                "type": "Expression",
                "targets": [
                    {"atom_type": "node", "type": "Symbol", "name": "Inheritance"},
                    {"atom_type": "variable", "name": "v1"},
                    {"atom_type": "node", "type": "Symbol", "name": '"mammal"'},
                ],
            },
            host=remote_das_host,
            port=get_remote_das_port(),
        )
        assert das.count_atoms({"precise": True}) == {
            "atom_count": 10,
            "node_count": 6,
            "link_count": 4,
        }
