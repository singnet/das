# flake8: noqa F811

from hyperon_das_atomdb.database import LinkT

import hyperon_das.link_filters as link_filter
from hyperon_das.client import FunctionsClient
from tests.python.integration.helpers import faas_fixture, metta_animal_base_handles


class TestVultrClientIntegration:
    def test_get_atom(
        self,
        faas_fixture: FunctionsClient,
    ):
        result = faas_fixture.get_atom(handle=metta_animal_base_handles.human)
        assert result.handle == metta_animal_base_handles.human
        assert result.name == '"human"'
        assert result.named_type == 'Symbol'

        result = faas_fixture.get_atom(handle=metta_animal_base_handles.monkey)
        assert result.handle == metta_animal_base_handles.monkey
        assert result.name == '"monkey"'
        assert result.named_type == 'Symbol'

        result = faas_fixture.get_atom(handle=metta_animal_base_handles.similarity_human_monkey)
        assert result.handle == metta_animal_base_handles.similarity_human_monkey
        assert result.named_type == 'Expression'
        assert result.targets == [
            metta_animal_base_handles.Similarity,
            metta_animal_base_handles.human,
            metta_animal_base_handles.monkey,
        ]

    def test_get_links(self, faas_fixture: FunctionsClient):  # noqa: F811
        links1 = faas_fixture.get_links(
            link_filter.FlatTypeTemplate(['Symbol', 'Symbol', 'Symbol'], 'Expression')
        )
        links2 = faas_fixture.get_links(link_filter.NamedType('Expression'))
        assert len(links1) == 43
        assert len(links2) == 43

    def test_count_atoms(self, faas_fixture: FunctionsClient):
        ret = faas_fixture.count_atoms()
        print(ret)
        assert ret == {'atom_count': 66}

    def test_query(self, faas_fixture: FunctionsClient):
        answer = faas_fixture.query(
            {
                "atom_type": "link",
                "type": "Expression",
                "targets": [
                    {"atom_type": "node", "type": "Symbol", "name": "Inheritance"},
                    {"atom_type": "variable", "name": "v1"},
                    {"atom_type": "variable", "name": "v2"},
                ],
            },
            {"no_iterator": True},
        )

        assert len(answer) == 12

        for link in answer:
            if link.subgraph.handle == metta_animal_base_handles.inheritance_human_mammal:
                break

        handles = [target.handle for target in link.subgraph.targets_documents]

        assert len(handles) == 3
        assert handles[1] == metta_animal_base_handles.human
        assert handles[2] == metta_animal_base_handles.mammal

        answer = faas_fixture.query(
            {
                "atom_type": "link",
                "type": "Expression",
                "targets": [
                    {"atom_type": "node", "type": "Symbol", "name": "Similarity"},
                    {"atom_type": "variable", "name": "v1"},
                    {"atom_type": "variable", "name": "v2"},
                ],
            },
            {"no_iterator": True},
        )

        assert len(answer) == 14

        for link in answer:
            if link.subgraph.handle == metta_animal_base_handles.similarity_human_monkey:
                break

        handles = [target.handle for target in link.subgraph.targets_documents]

        assert len(handles) == 3
        assert handles[1] == metta_animal_base_handles.human
        assert handles[2] == metta_animal_base_handles.monkey

    def test_get_incoming_links(self, faas_fixture: FunctionsClient):
        expected_handles = [
            metta_animal_base_handles.similarity_human_monkey,
            metta_animal_base_handles.similarity_human_chimp,
            metta_animal_base_handles.similarity_human_ent,
            metta_animal_base_handles.similarity_monkey_human,
            metta_animal_base_handles.similarity_chimp_human,
            metta_animal_base_handles.similarity_ent_human,
            metta_animal_base_handles.inheritance_human_mammal,
            metta_animal_base_handles.human_typedef,
        ]

        expected_atoms = [faas_fixture.get_atom(handle) for handle in expected_handles]

        expected_atoms_targets: dict[str, tuple[dict, list[dict]]] = {}
        for atom in expected_atoms:
            targets_documents: list[dict] = []
            for target in atom.targets:
                targets_documents.append(faas_fixture.get_atom(target).to_dict())
            expected_atoms_targets[atom.handle] = (atom.to_dict(), targets_documents)

        response_handles = faas_fixture.get_incoming_links(
            metta_animal_base_handles.human, targets_document=False, handles_only=True
        )
        assert all(isinstance(handle, str) for handle in response_handles)
        assert len(response_handles) == 8
        # response_handles has an extra link (named_type == MettaType) defining
        #     the metta type of '"human"'--> Type
        # response_handles has an extra link (named_type == Expression) defining
        #     the metta expression (: "human" Type)
        # assert len(set(response_handles).difference(set(expected_handles))) == 1
        response_handles = faas_fixture.get_incoming_links(
            metta_animal_base_handles.human, targets_document=True, handles_only=True
        )
        assert all(isinstance(handle, str) for handle in response_handles)
        assert len(response_handles) == 8
        # assert len(set(response_handles).difference(set(expected_handles))) == 1

        response_atoms = faas_fixture.get_incoming_links(
            metta_animal_base_handles.human, targets_document=False, handles_only=False
        )
        assert len(response_atoms) == 8
        for atom in response_atoms:
            assert isinstance(
                atom, LinkT
            ), f"Each item in body must be a LinkT instance. Received: {atom}"
            assert len(atom.targets) == 3
            assert atom.handle in [a.handle for a in expected_atoms]

        response_atoms = faas_fixture.get_incoming_links(metta_animal_base_handles.human)
        assert len(response_atoms) == 8
        for atom in response_atoms:
            assert isinstance(
                atom, LinkT
            ), f"Each item in body must be a LinkT instance. Received: {atom}"
            assert len(atom.targets) == 3
            assert atom.handle in [a.handle for a in expected_atoms]

        response_atoms_targets = faas_fixture.get_incoming_links(
            metta_animal_base_handles.human, targets_document=True, handles_only=False
        )
        assert len(response_atoms_targets) == 8
        for atom in response_atoms_targets:
            assert isinstance(
                atom, LinkT
            ), f"Each item in body must be a LinkT instance. Received: {atom}"
            atom_targets = [a.to_dict() for a in atom.targets_documents]
            assert len(atom_targets) == 3
            assert atom.handle in expected_atoms_targets
            for target in expected_atoms_targets[atom.handle][1]:
                assert target in atom_targets
