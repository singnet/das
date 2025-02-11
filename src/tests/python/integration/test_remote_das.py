# flake8: noqa F811

import pytest
from hyperon_das_atomdb import AtomDoesNotExist
from hyperon_das_atomdb.database import AtomT, NodeT

import hyperon_das.link_filters as link_filters
from hyperon_das import DistributedAtomSpace
from hyperon_das.exceptions import GetTraversalCursorException
from hyperon_das.traverse_engines import TraverseEngine
from tests.python.integration.helpers import remote_das_host  # noqa F401
from tests.python.integration.helpers import (
    das_remote_fixture_module,
    get_remote_das_port,
    metta_animal_base_handles,
)


def _check_docs(actual: list[AtomT], expected: list[dict]):
    assert len(actual) == len(expected)
    for doc1, doc2 in zip(actual, expected):
        doc1_as_dict = doc1.to_dict()
        for key in doc2.keys():
            assert doc1_as_dict[key] == doc2[key]
    return True


class TestRemoteDistributedAtomSpace:
    """Integration tests with OpenFaas function on the Vultr server. Using the Animal Knowledge Base"""

    def traversal(self, das: DistributedAtomSpace, handle: str):
        return das.get_traversal_cursor(handle)

    def test_server_connection(self, das_remote_fixture_module):
        try:
            das = das_remote_fixture_module
        except Exception as e:
            pytest.fail(f'Connection with OpenFaaS server fail, Details: {str(e)}')
        if not das.query_engine.remote_das.url:
            pytest.fail('Connection with server fail')
        assert (
            das.query_engine.remote_das.url
            == f'http://{remote_das_host}:{get_remote_das_port()}/function/query-engine'
        )

    def test_cache_controller(self, das_remote_fixture_module: DistributedAtomSpace):
        das_remote_fixture_module.cache_controller.atom_table["h1"] = {"handle": "h1"}
        assert das_remote_fixture_module.query_engine.get_atom("h1")["handle"] == "h1"

    def test_get_atom(self, das_remote_fixture_module: DistributedAtomSpace):
        result = das_remote_fixture_module.get_atom(handle=metta_animal_base_handles.human)
        assert result.handle == metta_animal_base_handles.human
        assert result.name == '"human"'
        assert result.named_type == 'Symbol'

        result = das_remote_fixture_module.get_atom(
            handle=metta_animal_base_handles.inheritance_dinosaur_reptile
        )
        assert result.handle == metta_animal_base_handles.inheritance_dinosaur_reptile
        assert result.named_type == 'Expression'
        assert result.targets == [
            metta_animal_base_handles.Inheritance,
            metta_animal_base_handles.dinosaur,
            metta_animal_base_handles.reptile,
        ]

        with pytest.raises(AtomDoesNotExist):
            das_remote_fixture_module.get_atom(handle='fake')

    @pytest.mark.skip("Wrong values")
    def test_get_links(self, das_remote_fixture_module: DistributedAtomSpace):
        all_inheritance = [
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
            metta_animal_base_handles.inheritance_typedef,
        ]

        links = das_remote_fixture_module.get_links(link_filters.NamedType('Expression'))
        inheritance_links = []
        for link in links:
            if metta_animal_base_handles.Inheritance in link.targets:
                inheritance_links.append(link.handle)
        assert len(inheritance_links) == 13
        assert sorted(inheritance_links) == sorted(all_inheritance)

        links = das_remote_fixture_module.get_links(
            link_filters.FlatTypeTemplate(['Symbol', 'Symbol', 'Symbol'], 'Expression')
        )
        inheritance_links = []
        for link in links:
            if metta_animal_base_handles.Inheritance in link.targets:
                inheritance_links.append(link.handle)
        assert len(inheritance_links) == 13
        assert sorted(inheritance_links) == sorted(all_inheritance)

        link = das_remote_fixture_module.get_links(
            link_filters.Targets(
                [
                    metta_animal_base_handles.Inheritance,
                    metta_animal_base_handles.earthworm,
                    metta_animal_base_handles.animal,
                ],
                'Expression',
            )
        )
        assert link[0].handle == metta_animal_base_handles.inheritance_earthworm_animal

    def test_get_incoming_links(self, das_remote_fixture_module: DistributedAtomSpace):
        expected_handles = [
            metta_animal_base_handles.inheritance_vine_plant,
            metta_animal_base_handles.similarity_snake_vine,
            metta_animal_base_handles.similarity_vine_snake,
            metta_animal_base_handles.vine_typedef,
        ]

        expected_atoms = [
            das_remote_fixture_module.get_atom(handle).to_dict() for handle in expected_handles
        ]

        response_atoms = das_remote_fixture_module.get_incoming_links(
            metta_animal_base_handles.vine, handles_only=False
        )
        for atom in response_atoms:
            if len(atom.targets) == 3:
                assert atom.to_dict() in expected_atoms

    @pytest.mark.skip("Wrong value, review")
    def test_count_atoms(self, das_remote_fixture_module: DistributedAtomSpace):
        response = das_remote_fixture_module.count_atoms(parameters={})
        assert response == {'atom_count': 66, 'node_count': 0, 'link_count': 0}
        response_local = das_remote_fixture_module.count_atoms({'context': 'local'})
        assert response_local == {'atom_count': 0, 'node_count': 0, 'link_count': 0}

    def test_query(self, das_remote_fixture_module: DistributedAtomSpace):
        all_inheritance_mammal = [
            metta_animal_base_handles.inheritance_chimp_mammal,
            metta_animal_base_handles.inheritance_human_mammal,
            metta_animal_base_handles.inheritance_monkey_mammal,
            metta_animal_base_handles.inheritance_rhino_mammal,
        ]

        query_answer = das_remote_fixture_module.query(
            {
                "atom_type": "link",
                "type": "Expression",
                "targets": [
                    {"atom_type": "node", "type": "Symbol", "name": "Inheritance"},
                    {"atom_type": "variable", "name": "v1"},
                    {"atom_type": "node", "type": "Symbol", "name": '"mammal"'},
                ],
            },
            {'no_iterator': True},
        )

        answer = tuple([item.assignment, item.subgraph] for item in query_answer)

        assert len(answer) == 4

        for _, link in answer:
            assert link.handle in all_inheritance_mammal
            if link.handle == metta_animal_base_handles.inheritance_chimp_mammal:
                assert _check_docs(
                    link.targets_documents,
                    [
                        {
                            'handle': metta_animal_base_handles.Inheritance,
                            'named_type': 'Symbol',
                            'name': "Inheritance",
                        },
                        {
                            'handle': metta_animal_base_handles.chimp,
                            'named_type': 'Symbol',
                            'name': '"chimp"',
                        },
                        {
                            'handle': metta_animal_base_handles.mammal,
                            'named_type': 'Symbol',
                            'name': '"mammal"',
                        },
                    ],
                )
            elif link.handle == metta_animal_base_handles.inheritance_human_mammal:
                assert _check_docs(
                    link.targets_documents,
                    [
                        {
                            'handle': metta_animal_base_handles.Inheritance,
                            'named_type': 'Symbol',
                            'name': "Inheritance",
                        },
                        {
                            'handle': metta_animal_base_handles.human,
                            'named_type': 'Symbol',
                            'name': '"human"',
                        },
                        {
                            'handle': metta_animal_base_handles.mammal,
                            'named_type': 'Symbol',
                            'name': '"mammal"',
                        },
                    ],
                )
            elif link.handle == metta_animal_base_handles.inheritance_monkey_mammal:
                assert _check_docs(
                    link.targets_documents,
                    [
                        {
                            'handle': metta_animal_base_handles.Inheritance,
                            'named_type': 'Symbol',
                            'name': "Inheritance",
                        },
                        {
                            'handle': metta_animal_base_handles.monkey,
                            'named_type': 'Symbol',
                            'name': '"monkey"',
                        },
                        {
                            'handle': metta_animal_base_handles.mammal,
                            'named_type': 'Symbol',
                            'name': '"mammal"',
                        },
                    ],
                )
            elif link.handle == metta_animal_base_handles.inheritance_rhino_mammal:
                assert _check_docs(
                    link.targets_documents,
                    [
                        {
                            'handle': metta_animal_base_handles.Inheritance,
                            'named_type': 'Symbol',
                            'name': "Inheritance",
                        },
                        {
                            'handle': metta_animal_base_handles.rhino,
                            'named_type': 'Symbol',
                            'name': '"rhino"',
                        },
                        {
                            'handle': metta_animal_base_handles.mammal,
                            'named_type': 'Symbol',
                            'name': '"mammal"',
                        },
                    ],
                )

    def test_get_traversal_cursor(self, das_remote_fixture_module: DistributedAtomSpace):
        cursor = das_remote_fixture_module.get_traversal_cursor(metta_animal_base_handles.human)
        assert cursor.get().handle == metta_animal_base_handles.human
        with pytest.raises(GetTraversalCursorException):
            das_remote_fixture_module.get_traversal_cursor('fake_handle')

    @pytest.mark.skip(reason="Disabled. Waiting for https://github.com/singnet/das/issues/73")
    def test_traverse_engine_methods(self, das_remote_fixture_module: DistributedAtomSpace):
        cursor: TraverseEngine = self.traversal(
            das_remote_fixture_module, metta_animal_base_handles.dinosaur
        )
        assert cursor.get().handle == metta_animal_base_handles.dinosaur

        def is_expression_link(link):
            return True if link['type'] == 'Expression' else False

        links_iter = cursor.get_links(filter=is_expression_link)

        expected_links = [
            das_remote_fixture_module.get_atom(handle)
            for handle in [
                metta_animal_base_handles.inheritance_dinosaur_reptile,
                metta_animal_base_handles.inheritance_triceratops_dinosaur,
                metta_animal_base_handles.dinosaur_typedef,
            ]
        ]

        count = len(expected_links)
        for link in links_iter:
            if len(link["targets"]) == 3:
                assert link in expected_links
                count -= 1
        assert count == 0

        def is_literal(atom: dict):
            return True if atom['is_literal'] is True else False

        neighbors_iter = cursor.get_neighbors(cursor_position=1, filter=is_literal)
        assert neighbors_iter.get()['handle'] == metta_animal_base_handles.reptile

        atom = cursor.follow_link(cursor_position=2, filter=is_literal)
        assert atom['handle'] == metta_animal_base_handles.triceratops

        cursor.goto(metta_animal_base_handles.human)
        assert cursor.get()['handle'] == metta_animal_base_handles.human

    @pytest.mark.skip('Wrong values')
    def test_fetch_atoms(self, das_remote_fixture_module):
        assert das_remote_fixture_module.backend.count_atoms() == {
            'atom_count': 0,
            'node_count': 0,
            'link_count': 0,
        }
        das_remote_fixture_module.fetch(
            query={
                "atom_type": "link",
                "type": "Expression",
                "targets": [
                    {"atom_type": "node", "type": "Symbol", "name": "Inheritance"},
                    {"atom_type": "variable", "name": "v1"},
                    {"atom_type": "node", "type": "Symbol", "name": '"mammal"'},
                ],
            }
        )
        assert das_remote_fixture_module.backend.count_atoms() == {
            'atom_count': 10,
            'node_count': 6,
            'link_count': 4,
        }

    @pytest.mark.skip('Wrong values')
    def test_fetch_all_data(self, das_remote_fixture_module):
        assert das_remote_fixture_module.backend.count_atoms() == {
            'atom_count': 0,
            'node_count': 0,
            'link_count': 0,
        }
        das_remote_fixture_module.fetch()
        assert das_remote_fixture_module.backend.count_atoms() == {
            'atom_count': 66,
            'node_count': 23,
            'link_count': 43,
        }

    def test_create_context(self, das_remote_fixture_module):
        context_name = 'my context'
        context = das_remote_fixture_module.create_context(context_name)
        assert context.name == context_name

    @pytest.mark.skip(reason="Disable. See https://github.com/singnet/das-query-engine/issues/259")
    def test_commit_changes(self, das_remote_fixture_module: DistributedAtomSpace):
        node = das_remote_fixture_module.get_atom(handle=metta_animal_base_handles.human)
        assert hasattr(node, 'test_key') is False
        assert 'test_key' not in node.custom_attributes
        das_remote_fixture_module.add_node(NodeT(**{'type': 'Symbol', 'name': '"human"'}))
        das_remote_fixture_module.commit_changes()
        node = das_remote_fixture_module.get_atom(handle=metta_animal_base_handles.human)
        assert node.custom_attributes['test_key'] == 'test_value'

    def test_commit_changes_method_with_mode_parameter(self, das_remote_fixture_module):
        das = das_remote_fixture_module

        with pytest.raises(PermissionError):
            das_remote_fixture_module
            das.commit_changes()

        with pytest.raises(ValueError):
            das = DistributedAtomSpace(
                mode='blah', query_engine='remote', host=remote_das_host, port=get_remote_das_port()
            )
            das.commit_changes()
