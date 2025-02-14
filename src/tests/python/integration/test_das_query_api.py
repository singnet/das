import pytest

from hyperon_das import DistributedAtomSpace
from tests.python.integration.helpers import get_remote_das_port, remote_das_host

from .helpers import (
    MettaAnimalBaseHandlesCollection,
    _db_down,
    _db_up,
    remote_up,
    remote_down,
    cleanup,
    load_metta_animals_base,
    mongo_port,
    redis_port,
)

das_instance = {}


class TestDASQueryAPI:
    """Test query methods in DAS API with integration of DAS and its various AtomDB adapters"""

    @classmethod
    def setup_class(cls):
        _db_up()
        remote_up(build=False)

        das_instance["local_ram"] = DistributedAtomSpace(query_engine='local', atomdb='ram')
        load_metta_animals_base(das_instance["local_ram"])

        das_instance["local_redis_mongo"] = DistributedAtomSpace(
            query_engine='local',
            atomdb='redis_mongo',
            mongo_port=mongo_port,
            mongo_username='dbadmin',
            mongo_password='dassecret',
            redis_port=redis_port,
            redis_cluster=False,
            redis_ssl=False,
        )
        load_metta_animals_base(das_instance["local_redis_mongo"])
        das_instance["local_redis_mongo"].commit_changes()

        das_instance["remote"] = DistributedAtomSpace(
            query_engine='remote', host=remote_das_host, port=get_remote_das_port()
        )

    @classmethod
    def teardown_class(cls):
        _db_down()
        remote_down(no_destroy=True)

    @pytest.fixture(scope="session", autouse=True)
    def _cleanup(self, request):
        return cleanup(request)

    def test_count_atoms(self):
        for key, das in das_instance.items():
            count = das.count_atoms({})
            assert count["atom_count"] == 66

    def test_get_atom(self):
        for key, das in das_instance.items():
            node_list = []
            for handle in MettaAnimalBaseHandlesCollection.node_handles:
                atom = das.get_atom(handle)
                assert atom.handle == handle
                node_list.append(atom.to_dict())
            link_list = []
            for handle in MettaAnimalBaseHandlesCollection.link_handles:
                atom = das.get_atom(handle)
                assert atom.handle == handle
                link_list.append(atom.to_dict())
            if key != "remote":
                assert node_list == [
                    atom.to_dict()
                    for atom in das.get_atoms(MettaAnimalBaseHandlesCollection.node_handles)
                ]
                assert link_list == [
                    atom.to_dict()
                    for atom in das.get_atoms(MettaAnimalBaseHandlesCollection.link_handles)
                ]
