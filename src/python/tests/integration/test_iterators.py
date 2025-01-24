# flake8: noqa F811

import pytest

from hyperon_das import DistributedAtomSpace
from hyperon_das.cache import QueryAnswerIterator
from hyperon_das.cache.iterators import CustomQuery, TraverseNeighborsIterator
from tests.integration.helpers import (
    _db_down,
    _db_up,
    cleanup,
    das_local_fixture_class,
    das_remote_fixture_module,
    get_remote_das_port,
    load_metta_animals_base,
    metta_animal_base_handles,
    mongo_port,
    redis_port,
    remote_das_host,
)


@pytest.mark.skip(
    reason="Waiting for integration with cache sub-module https://github.com/singnet/das/issues/73"
)
class TestTraverseLinks:
    @pytest.fixture(scope="session")
    def _cleanup(self, request):
        return cleanup(request)

    @pytest.fixture
    def human_handle(self):
        return metta_animal_base_handles.human

    def _is_expression_atom(self, atom: dict) -> bool:
        if atom['named_type'] != 'Expression':
            return False
        return True

    def _human_incoming_links(self):
        return sorted(
            [
                metta_animal_base_handles.similarity_human_monkey,
                metta_animal_base_handles.similarity_monkey_human,
                metta_animal_base_handles.similarity_human_chimp,
                metta_animal_base_handles.similarity_chimp_human,
                metta_animal_base_handles.similarity_human_ent,
                metta_animal_base_handles.similarity_ent_human,
                metta_animal_base_handles.inheritance_human_mammal,
                metta_animal_base_handles.human_typedef,
            ]
        )

    def _check_asserts(self, das: DistributedAtomSpace, iterator: QueryAnswerIterator):
        current_value = iterator.get()
        assert current_value == das.query_engine.get_atom(
            iterator.get()['handle'], targets_document=True
        )
        assert isinstance(current_value, dict)
        assert iterator.is_empty() is False
        link_handles = sorted([item['handle'] for item in iterator])
        assert len(link_handles) == 8
        assert link_handles == self._human_incoming_links()
        assert iterator.is_empty() is True
        with pytest.raises(StopIteration):
            iterator.get()
        with pytest.raises(StopIteration):
            next(iterator)

    def test_traverse_links_with_das_ram_only(self, human_handle):
        das = DistributedAtomSpace()
        load_metta_animals_base(das)
        traverse = das.get_traversal_cursor(human_handle)
        iterator = traverse.get_links(filter=self._is_expression_atom)
        self._check_asserts(das, iterator)

    def test_traverse_links_with_das_redis_mongo(self, human_handle, _cleanup):
        _db_up()
        das = DistributedAtomSpace(
            query_engine='local',
            atomdb='redis_mongo',
            mongo_port=mongo_port,
            mongo_username='dbadmin',
            mongo_password='dassecret',
            redis_port=redis_port,
            redis_cluster=False,
            redis_ssl=False,
        )
        load_metta_animals_base(das)
        das.commit_changes()
        traverse = das.get_traversal_cursor(human_handle)
        iterator = traverse.get_links(filter=self._is_expression_atom)
        self._check_asserts(das, iterator)
        _db_down()

    def test_traverse_links_with_remote_das(
        self, human_handle, das_remote_fixture: DistributedAtomSpace
    ):
        das = das_remote_fixture
        traverse = das.get_traversal_cursor(human_handle)
        iterator = traverse.get_links(filter=self._is_expression_atom)
        self._check_asserts(das, iterator)


@pytest.mark.skip(
    reason="Waiting for integration with cache sub-module https://github.com/singnet/das/issues/73"
)
class TestTraverseNeighbors:
    @pytest.fixture(scope="session")
    def _cleanup(self, request):
        return cleanup(request)

    @pytest.fixture
    def human_handle(self):
        return metta_animal_base_handles.human

    def _human_neighbors(self):
        return sorted(
            [
                metta_animal_base_handles.mammal,
                metta_animal_base_handles.monkey,
                metta_animal_base_handles.chimp,
                metta_animal_base_handles.ent,
            ]
        )

    def _is_literal_atom(self, atom: dict) -> bool:
        if atom['is_literal'] is False:
            return False
        return True

    def _check_asserts(self, das: DistributedAtomSpace, iterator: TraverseNeighborsIterator):
        current_value = iterator.get()
        assert current_value == das.get_atom(iterator.get()['handle'])
        assert isinstance(current_value, dict)
        assert iterator.is_empty() is False
        node_handles = sorted([item['handle'] for item in iterator])
        assert len(node_handles) == 4
        assert node_handles == self._human_neighbors()
        assert iterator.is_empty() is True
        with pytest.raises(StopIteration):
            iterator.get()
        with pytest.raises(StopIteration):
            next(iterator)

    def test_traverse_neighbors_with_das_ram_only(self, human_handle):
        das = DistributedAtomSpace()
        load_metta_animals_base(das)
        traverse = das.get_traversal_cursor(human_handle)
        iterator = traverse.get_neighbors(filters=(None, self._is_literal_atom))
        self._check_asserts(das, iterator)

    def test_traverse_neighbors_with_das_redis_mongo(self, human_handle, _cleanup):
        _db_up()
        das = DistributedAtomSpace(
            query_engine='local',
            atomdb='redis_mongo',
            mongo_port=mongo_port,
            mongo_username='dbadmin',
            mongo_password='dassecret',
            redis_port=redis_port,
            redis_cluster=False,
            redis_ssl=False,
        )
        load_metta_animals_base(das)
        das.commit_changes()
        traverse = das.get_traversal_cursor(human_handle)
        iterator = traverse.get_neighbors(filters=(None, self._is_literal_atom))
        self._check_asserts(das, iterator)
        _db_down()

    def test_traverse_neighbors_with_remote_das(
        self, human_handle, das_remote_fixture: DistributedAtomSpace
    ):
        das = das_remote_fixture
        traverse = das.get_traversal_cursor(human_handle)
        iterator = traverse.get_neighbors(filters=(None, self._is_literal_atom))
        self._check_asserts(das, iterator)


class TestCustomQuery:
    @pytest.fixture(scope="session")
    def _cleanup(self, request):
        return cleanup(request)

    def _all_nodes(self):
        return sorted(
            [
                metta_animal_base_handles.human,
                metta_animal_base_handles.monkey,
                metta_animal_base_handles.chimp,
                metta_animal_base_handles.mammal,
                metta_animal_base_handles.animal,
                metta_animal_base_handles.reptile,
                metta_animal_base_handles.snake,
                metta_animal_base_handles.dinosaur,
                metta_animal_base_handles.earthworm,
                metta_animal_base_handles.rhino,
                metta_animal_base_handles.plant,
                metta_animal_base_handles.vine,
                metta_animal_base_handles.ent,
                metta_animal_base_handles.triceratops,
            ]
        )

    def _all_links(self):
        return sorted(
            [
                metta_animal_base_handles.similarity_human_monkey,
                metta_animal_base_handles.similarity_human_chimp,
                metta_animal_base_handles.similarity_chimp_monkey,
                metta_animal_base_handles.similarity_snake_earthworm,
                metta_animal_base_handles.similarity_rhino_triceratops,
                metta_animal_base_handles.similarity_snake_vine,
                metta_animal_base_handles.similarity_human_ent,
                metta_animal_base_handles.inheritance_human_mammal,
                metta_animal_base_handles.inheritance_monkey_mammal,
                metta_animal_base_handles.inheritance_chimp_mammal,
                metta_animal_base_handles.inheritance_mammal_animal,
                metta_animal_base_handles.inheritance_reptile_animal,
                metta_animal_base_handles.inheritance_snake_reptile,
                metta_animal_base_handles.inheritance_dinosaur_reptile,
                metta_animal_base_handles.inheritance_triceratops_dinosaur,
                metta_animal_base_handles.inheritance_earthworm_animal,
                metta_animal_base_handles.inheritance_rhino_mammal,
                metta_animal_base_handles.inheritance_vine_plant,
                metta_animal_base_handles.inheritance_ent_plant,
                metta_animal_base_handles.similarity_monkey_human,
                metta_animal_base_handles.similarity_chimp_human,
                metta_animal_base_handles.similarity_monkey_chimp,
                metta_animal_base_handles.similarity_earthworm_snake,
                metta_animal_base_handles.similarity_triceratops_rhino,
                metta_animal_base_handles.similarity_vine_snake,
                metta_animal_base_handles.similarity_ent_human,
                metta_animal_base_handles.human_typedef,
                metta_animal_base_handles.monkey_typedef,
                metta_animal_base_handles.chimp_typedef,
                metta_animal_base_handles.snake_typedef,
                metta_animal_base_handles.earthworm_typedef,
                metta_animal_base_handles.rhino_typedef,
                metta_animal_base_handles.triceratops_typedef,
                metta_animal_base_handles.vine_typedef,
                metta_animal_base_handles.ent_typedef,
                metta_animal_base_handles.mammal_typedef,
                metta_animal_base_handles.animal_typedef,
                metta_animal_base_handles.reptile_typedef,
                metta_animal_base_handles.dinosaur_typedef,
                metta_animal_base_handles.plant_typedef,
                metta_animal_base_handles.similarity_typedef,
                metta_animal_base_handles.inheritance_typedef,
                metta_animal_base_handles.concept_typedef,
            ]
        )

    def _asserts(self, das: DistributedAtomSpace):
        node_index = das.create_field_index(
            atom_type='node', fields=['is_literal'], named_type='Symbol'
        )
        link_index_type = das.create_field_index(
            atom_type='link', fields=['is_toplevel'], named_type='Expression'
        )
        link_index_composite_type = das.create_field_index(
            atom_type='link',
            fields=['is_toplevel'],
            composite_type=['Expression', 'Symbol', 'Symbol', 'Symbol'],
        )

        node_iterator = das.custom_query(node_index, query={'is_literal': True}, no_iterator=False)
        link_iterator_type = das.custom_query(
            link_index_type,
            query={'is_toplevel': True},
            chunk_size=10,
            no_iterator=False,
        )
        link_iterator_composite_type = das.custom_query(
            link_index_composite_type,
            query={'is_toplevel': True},
            chunk_size=5,
            no_iterator=False,
        )

        nodes = self._check_asserts(das, node_iterator)
        links_type = self._check_asserts(das, link_iterator_type)
        links_composite_type = self._check_asserts(das, link_iterator_composite_type)
        assert sorted(nodes) == self._all_nodes()
        assert sorted(links_type) == self._all_links()
        assert sorted(links_composite_type) == self._all_links()

    def _check_asserts(self, das: DistributedAtomSpace, iterator: CustomQuery):
        current_value = iterator.get()
        assert current_value == das.get_atom(iterator.get()['handle'])
        assert isinstance(current_value, dict)
        assert iterator.is_empty() is False
        handles = sorted([item['handle'] for item in iterator])
        assert iterator.is_empty() is True
        with pytest.raises(StopIteration):
            iterator.get()
        with pytest.raises(StopIteration):
            next(iterator)
        return handles

    @pytest.mark.skip(
        "Requires Mongo indices to be updated with the new custom attributes. See https://github.com/singnet/das-query-engine/issues/357"
    )
    def test_custom_query_with_local_das_redis_mongo(self, _cleanup, das_local_fixture_class):
        das = das_local_fixture_class
        load_metta_animals_base(das)
        das.commit_changes()
        self._asserts(das)

    @pytest.mark.skip("Waiting fix")
    def test_custom_query_with_remote_das(self):
        das = DistributedAtomSpace(
            query_engine='remote', host=remote_das_host, port=get_remote_das_port()
        )
        self._asserts(das)

    def test_get_atom_by_field_local(self, das_local_fixture_class):
        das = das_local_fixture_class
        load_metta_animals_base(das)
        das.commit_changes()
        atom_field = das.get_atoms_by_field({'name': '"chimp"'})
        assert atom_field

    def test_get_atoms_by_field_remote(self, das_remote_fixture_module):
        das = das_remote_fixture_module
        atom_field = das.get_atoms_by_field({'name': '"chimp"'})
        assert atom_field

    def test_get_atoms_by_text_field_local(self, das_local_fixture_class):
        das = das_local_fixture_class
        load_metta_animals_base(das)
        das.commit_changes()
        with pytest.raises(Exception, match=r'text index required for \$text query'):
            das.get_atoms_by_text_field(text_value='"')
        atom_text_field = das.get_atoms_by_text_field(text_value='"chim', field='name')
        assert atom_text_field

    def test_get_atoms_by_text_field_remote(self, das_remote_fixture_module):
        das = das_remote_fixture_module
        atom_text_field = das.get_atoms_by_text_field(text_value='"chim', field='name')
        assert atom_text_field

    def test_get_atoms_starting_local(self, das_local_fixture_class):
        das = das_local_fixture_class
        load_metta_animals_base(das)
        das.commit_changes()
        atom_starting_with = das.get_node_by_name_starting_with('Symbol', '"mon')
        assert atom_starting_with

    def test_get_atoms_starting_remote(self, das_remote_fixture_module):
        das = das_remote_fixture_module
        atom_starting_with = das.get_node_by_name_starting_with('Symbol', '"mon')
        assert atom_starting_with
