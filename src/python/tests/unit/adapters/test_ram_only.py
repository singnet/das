from typing import Any

import pytest

from hyperon_das_atomdb import AtomDB
from hyperon_das_atomdb.adapters import InMemoryDB
from hyperon_das_atomdb.database import LinkT, NodeT
from hyperon_das_atomdb.exceptions import AddLinkException, AddNodeException, AtomDoesNotExist
from hyperon_das_atomdb.utils.expression_hasher import ExpressionHasher
from tests.helpers import dict_to_link_params, dict_to_node_params
from tests.unit.fixtures import in_memory_db  # noqa: F401


class TestInMemoryDB:
    all_added_nodes = []
    all_added_links = []

    @pytest.fixture()
    def database(self, in_memory_db):  # noqa: F811
        import json
        import pathlib

        path = pathlib.Path(__file__).parent.resolve()

        db = in_memory_db
        with open(f"{path}/data/ram_only_nodes.json") as f:
            all_nodes: list[dict[str, Any]] = json.load(f)
        with open(f"{path}/data/ram_only_links.json") as f:
            all_links: list[dict[str, Any]] = json.load(f)
        self.all_added_nodes = [db.add_node(dict_to_node_params(node)) for node in all_nodes]
        self.all_added_links = [db.add_link(dict_to_link_params(link)) for link in all_links]
        yield db

    @pytest.mark.parametrize(
        "node_type,node_name,expected",
        [
            ("Concept", "human", "af12f10f9ae2002a1607ba0b47ba8407"),
            ("Concept", "monkey", "1cdffc6b0b89ff41d68bec237481d1e1"),
            ("Concept", "chimp", "5b34c54bee150c04f9fa584b899dc030"),
            ("Concept", "snake", "c1db9b517073e51eb7ef6fed608ec204"),
            ("Concept", "earthworm", "bb34ce95f161a6b37ff54b3d4c817857"),
            ("Concept", "rhino", "99d18c702e813b07260baf577c60c455"),
            ("Concept", "triceratops", "d03e59654221c1e8fcda404fd5c8d6cb"),
            ("Concept", "vine", "b94941d8cd1c0ee4ad3dd3dcab52b964"),
            ("Concept", "ent", "4e8e26e3276af8a5c2ac2cc2dc95c6d2"),
            ("Concept", "mammal", "bdfe4e7a431f73386f37c6448afe5840"),
            ("Concept", "animal", "0a32b476852eeb954979b87f5f6cb7af"),
            ("Concept", "reptile", "b99ae727c787f1b13b452fd4c9ce1b9a"),
            ("Concept", "dinosaur", "08126b066d32ee37743e255a2558cccd"),
            ("Concept", "plant", "80aff30094874e75028033a38ce677bb"),
        ],
    )
    def test_get_node_handle(self, node_type, node_name, expected, request):
        db: InMemoryDB = request.getfixturevalue("database")
        actual = db.get_node_handle(node_type=node_type, node_name=node_name)
        assert expected == actual

    @pytest.mark.parametrize(
        "node_type,node_name",
        [
            (
                "Concept-Fake",
                "fake",
            ),
            ("concept", "monkey"),
            ("Concept", "Monkey"),
            ("Concept", "mnkêy"),
        ],
    )
    def test_get_node_handle_not_exist(self, node_type, node_name, database: InMemoryDB):
        with pytest.raises(AtomDoesNotExist, match="Nonexistent atom"):
            database.get_node_handle(node_type=node_type, node_name=node_name)

    @pytest.mark.parametrize(
        "targets,link_type,expected",
        [
            (
                [("Concept", "human"), ("Concept", "chimp")],
                "Similarity",
                "b5459e299a5c5e8662c427f7e01b3bf1",
            ),
            (
                [("Concept", "ent"), ("Concept", "plant")],
                "Inheritance",
                "ee1c03e6d1f104ccd811cfbba018451a",
            ),
            (
                [("Concept", "ent"), ("Concept", "human")],
                "Similarity",
                "a45af31b43ee5ea271214338a5a5bd61",
            ),
        ],
    )
    def test_get_link_handle(self, targets, link_type, expected, request):
        db: InMemoryDB = request.getfixturevalue("database")
        a = db.get_node_handle(*targets[0])
        b = db.get_node_handle(*targets[1])
        actual = db.get_link_handle(link_type=link_type, target_handles=[a, b])
        assert expected == actual

    @pytest.mark.parametrize(
        "link_type,target_handles",
        [
            ("Singularity-Fake", ["Fake-1", "Fake-2"]),
            ("Singularity", ["Fake-1", "Fake-2"]),
            ("singularity", ["monkey", "mammal"]),
            ("Sing", ["mnkêy", "âêéôç"]),
        ],
    )
    def test_get_link_handle_not_exist(self, link_type, target_handles, database: InMemoryDB):
        with pytest.raises(AtomDoesNotExist, match="Nonexistent atom"):
            database.get_link_handle(link_type=link_type, target_handles=target_handles)

    def test_node_exists_true(self, database: InMemoryDB):
        ret = database.node_exists(node_type="Concept", node_name="human")
        assert ret is True

    def test_node_exists_false(self, database: InMemoryDB):
        ret = database.node_exists(node_type="Concept-Fake", node_name="human-fake")
        assert ret is False

    def test_link_exists_true(self, database: InMemoryDB):
        human = database.get_node_handle("Concept", "human")
        monkey = database.get_node_handle("Concept", "monkey")
        ret = database.link_exists(link_type="Similarity", target_handles=[human, monkey])
        assert ret is True

    def test_link_exists_false(self, database: InMemoryDB):
        ret = database.link_exists(link_type="Concept-Fake", target_handles=["Fake1, Fake2"])
        assert ret is False

    @pytest.mark.parametrize(
        "targets,link_type",
        [
            ([("Concept", "human"), ("Concept", "mammal")], "Inheritance"),
            ([("Concept", "ent"), ("Concept", "plant")], "Inheritance"),
            ([("Concept", "ent"), ("Concept", "human")], "Similarity"),
        ],
    )
    def test_get_link_targets(self, targets, link_type, database: InMemoryDB):
        target_handles = [database.get_node_handle(*t) for t in targets]
        handle = database.get_link_handle(link_type, target_handles)
        ret = database.get_link_targets(handle)
        assert ret is not None
        assert ret == target_handles

    def test_get_link_targets_invalid(self, database: InMemoryDB):
        with pytest.raises(AtomDoesNotExist, match="Nonexistent atom"):
            database.get_link_targets("link_handle_Fake")

    @pytest.mark.parametrize(
        "targets,link_type,expected",
        [
            (
                [("Concept", "human"), ("Concept", "monkey")],
                "Similarity",
                {"bad7472f41a0e7d601ca294eb4607c3a"},
            ),
            (
                [("Concept", "human"), ("Concept", "mammal")],
                "Inheritance",
                {"c93e1e758c53912638438e2a7d7f7b7f"},
            ),
            (
                [("Concept", "ent"), ("Concept", "plant")],
                "Inheritance",
                {"ee1c03e6d1f104ccd811cfbba018451a"},
            ),
            (
                [("Concept", "ent"), ("Concept", "human")],
                "Similarity",
                {"a45af31b43ee5ea271214338a5a5bd61"},
            ),
            (
                [("Concept", "human"), ("Concept", "monkey")],
                "*",
                {"bad7472f41a0e7d601ca294eb4607c3a"},
            ),
            (
                [("Concept", "human"), ("Concept", "mammal")],
                "*",
                {"c93e1e758c53912638438e2a7d7f7b7f"},
            ),
            ([("Concept", "ent"), ("Concept", "plant")], "*", {"ee1c03e6d1f104ccd811cfbba018451a"}),
            ([("Concept", "ent"), ("Concept", "human")], "*", {"a45af31b43ee5ea271214338a5a5bd61"}),
            (
                [("Concept", "human"), ("Concept", "chimp")],
                "*",
                {"b5459e299a5c5e8662c427f7e01b3bf1"},
            ),
            (
                ["*", ("Concept", "chimp")],
                "Similarity",
                {"b5459e299a5c5e8662c427f7e01b3bf1", "31535ddf214f5b239d3b517823cb8144"},
            ),
            (
                ["*", ("Concept", "human")],
                "*",
                {
                    "2c927fdc6c0f1272ee439ceb76a6d1a4",
                    "2a8a69c01305563932b957de4b3a9ba6",
                    "a45af31b43ee5ea271214338a5a5bd61",
                },
            ),
            (
                [("Concept", "chimp"), "*"],
                "Similarity",
                {"abe6ad743fc81bd1c55ece2e1307a178", "2c927fdc6c0f1272ee439ceb76a6d1a4"},
            ),
            (
                [("Concept", "chimp"), "*"],
                "*",
                {
                    "abe6ad743fc81bd1c55ece2e1307a178",
                    "2c927fdc6c0f1272ee439ceb76a6d1a4",
                    "75756335011dcedb71a0d9a7bd2da9e8",
                },
            ),
        ],
    )
    def test_get_matched_links(self, targets, link_type, expected, database: InMemoryDB):
        target_handles = [database.get_node_handle(*t) if t != "*" else t for t in targets]
        actual = database.get_matched_links(link_type, target_handles)
        assert actual
        assert isinstance(actual, set)
        assert expected == actual

    def test_get_matched_links_link_does_not_exist(self, database: InMemoryDB):
        link_type = "Similarity-Fake"
        chimp = ExpressionHasher.terminal_hash("Concept", "chimp")
        assert database.get_matched_links(link_type, [chimp, chimp]) == set()

    def test_get_matched_links_toplevel_only(self, database: InMemoryDB):
        database.add_link(
            dict_to_link_params(
                {
                    "type": "Evaluation",
                    "targets": [
                        {"type": "Predicate", "name": "Predicate:has_name"},
                        {
                            "type": "Evaluation",
                            "targets": [
                                {
                                    "type": "Predicate",
                                    "name": "Predicate:has_name",
                                },
                                {
                                    "targets": [
                                        {
                                            "type": "Reactome",
                                            "name": "Reactome:R-HSA-164843",
                                        },
                                        {
                                            "type": "Concept",
                                            "name": "Concept:2-LTR circle formation",
                                        },
                                    ],
                                    "type": "Set",
                                },
                            ],
                        },
                    ],
                }
            )
        )
        expected = {"661fb5a7c90faabfeada7e1f63805fc0"}
        actual = database.get_matched_links("Evaluation", ["*", "*"], toplevel_only=True)

        assert expected == actual

    def test_get_matched_links_wrong_parameter(self, database: InMemoryDB):
        database.add_link(
            dict_to_link_params(
                {
                    "type": "Evaluation",
                    "targets": [
                        {"type": "Predicate", "name": "Predicate:has_name"},
                        {
                            "type": "Evaluation",
                            "targets": [
                                {
                                    "type": "Predicate",
                                    "name": "Predicate:has_name",
                                },
                                {
                                    "targets": [
                                        {
                                            "type": "Reactome",
                                            "name": "Reactome:R-HSA-164843",
                                        },
                                        {
                                            "type": "Concept",
                                            "name": "Concept:2-LTR circle formation",
                                        },
                                    ],
                                    "type": "Set",
                                },
                            ],
                        },
                    ],
                }
            )
        )
        actual = database.get_matched_links("Evaluation", ["*", "*"], toplevel=True)
        assert len(actual) == 2

    @pytest.mark.skip(
        reason=(
            "get_matched_links does not support nested lists in the target_handles parameter. "
            "See: https://github.com/singnet/das-atom-db/issues/191"
        )
    )
    def test_get_matched_links_nested_lists(self, database: InMemoryDB):
        database.add_link(
            dict_to_link_params(
                {
                    "type": "Connectivity",
                    "targets": [
                        {
                            "type": "Nearness",
                            "targets": [
                                {"type": "Concept", "name": "chimp"},
                                {"type": "Concept", "name": "human"},
                            ],
                        },
                        {
                            "type": "Nearness",
                            "targets": [
                                {"type": "Concept", "name": "chimp"},
                                {"type": "Concept", "name": "monkey"},
                            ],
                        },
                    ],
                }
            )
        )
        chimp = ExpressionHasher.terminal_hash("Concept", "chimp")
        human = ExpressionHasher.terminal_hash("Concept", "human")
        monkey = ExpressionHasher.terminal_hash("Concept", "monkey")
        assert database.link_exists("Nearness", [chimp, human])
        assert database.link_exists("Nearness", [chimp, monkey])
        nearness_chimp_human_handle = database.get_link_handle("Nearness", [chimp, human])
        nearness_chimp_monkey_handle = database.get_link_handle("Nearness", [chimp, monkey])
        assert database.link_exists(
            "Connectivity",
            [nearness_chimp_human_handle, nearness_chimp_monkey_handle],
        )
        target_handles = [[chimp, human], [chimp, monkey]]
        _, links = database.get_matched_links("Connectivity", target_handles)
        assert len(links) == 1

    def test_get_all_nodes(self, database):
        ret = database.get_all_nodes_handles("Concept")
        assert len(ret) == 14
        ret = database.get_all_nodes_names("Concept")
        assert len(ret) == 14
        ret = database.get_all_nodes_handles("ConceptFake")
        assert len(ret) == 0

    def test_get_matched_type_template(self, database: InMemoryDB):
        v1 = database.get_matched_type_template(["Inheritance", "Concept", "Concept"])
        v2 = database.get_matched_type_template(["Similarity", "Concept", "Concept"])
        v3 = database.get_matched_type_template(["Inheritance", "Concept", "blah"])
        v4 = database.get_matched_type_template(["Similarity", "blah", "Concept"])
        v5 = database.get_matched_links("Inheritance", ["*", "*"])
        v6 = database.get_matched_links("Similarity", ["*", "*"])
        assert len(v1) == 12
        assert len(v2) == 14
        assert len(v3) == 0
        assert len(v4) == 0
        assert v1 == v5
        assert v2 == v6

    def test_get_matched_type_template_toplevel_only(self, database: InMemoryDB):
        database.add_link(
            dict_to_link_params(
                {
                    "type": "Evaluation",
                    "targets": [
                        {"type": "Predicate", "name": "Predicate:has_name"},
                        {
                            "type": "Evaluation",
                            "targets": [
                                {
                                    "type": "Reactome",
                                    "name": "Reactome:R-HSA-164843",
                                },
                                {
                                    "type": "Concept",
                                    "name": "Concept:2-LTR circle formation",
                                },
                            ],
                        },
                    ],
                }
            )
        )

        ret = database.get_matched_type_template(
            ["Evaluation", "Reactome", "Concept"], toplevel_only=True
        )
        assert len(ret) == 0

        ret = database.get_matched_type_template(
            ["Evaluation", "Reactome", "Concept"], toplevel_only=False
        )
        assert len(ret) == 1

    def test_get_matched_type(self, database: InMemoryDB):
        inheritance = database.get_matched_type("Inheritance")
        similarity = database.get_matched_type("Similarity")
        assert len(inheritance) == 12
        assert len(similarity) == 14

    def test_get_matched_type_toplevel_only(self, database: InMemoryDB):
        database.add_link(
            dict_to_link_params(
                {
                    "type": "EvaluationLink",
                    "targets": [
                        {"type": "Predicate", "name": "Predicate:has_name"},
                        {
                            "type": "EvaluationLink",
                            "targets": [
                                {
                                    "type": "Reactome",
                                    "name": "Reactome:R-HSA-164843",
                                },
                                {
                                    "type": "Concept",
                                    "name": "Concept:2-LTR circle formation",
                                },
                            ],
                        },
                    ],
                }
            )
        )
        ret = database.get_matched_type("EvaluationLink")
        assert len(ret) == 2

        ret = database.get_matched_type("EvaluationLink", toplevel_only=True)
        assert len(ret) == 1

    def test_get_node_name(self, database):
        handle = database.get_node_handle("Concept", "monkey")
        db_name = database.get_node_name(handle)

        assert db_name == "monkey"

    def test_get_node_name_error(self, database):
        with pytest.raises(AtomDoesNotExist, match="Nonexistent atom"):
            database.get_node_name("handle-test")

    def test_get_node_type(self, database):
        handle = database.get_node_handle("Concept", "monkey")
        db_type = database.get_node_type(handle)

        assert db_type == "Concept"

    def test_get_node_type_error(self, database):
        with pytest.raises(AtomDoesNotExist, match="Nonexistent atom"):
            database.get_node_type("handle-test")

    def test_get_matched_node_name(self, database: InMemoryDB):
        expected = sorted(
            [
                database.get_node_handle("Concept", "human"),
                database.get_node_handle("Concept", "mammal"),
                database.get_node_handle("Concept", "animal"),
            ]
        )
        actual = sorted(database.get_node_by_name("Concept", "ma"))

        assert expected == actual
        assert sorted(database.get_node_by_name("blah", "Concept")) == []
        assert sorted(database.get_node_by_name("Concept", "blah")) == []

    def test_add_node_without_type_parameter(self, database: InMemoryDB):
        with pytest.raises(AddNodeException, match="'type' and 'name' are required."):
            database.add_node(dict_to_node_params({"type": "", "name": "car"}))

    def test_add_node_without_name_parameter(self, database: InMemoryDB):
        with pytest.raises(AddNodeException, match="'type' and 'name' are required."):
            database.add_node(dict_to_node_params({"type": "Concept", "name": ""}))

    def test_add_node(self, database: InMemoryDB):
        assert len(database.get_all_nodes_handles("Concept")) == 14
        database.add_node(dict_to_node_params({"type": "Concept", "name": "car"}))
        assert len(database.get_all_nodes_handles("Concept")) == 15
        node_handle = database.get_node_handle("Concept", "car")
        node_name = database.get_node_name(node_handle)
        assert node_name == "car"

    def test_add_link_without_type_parameter(self, database: InMemoryDB):
        with pytest.raises(AddLinkException, match="'type' and 'targets' are required."):
            database.add_link(
                dict_to_link_params(
                    {
                        "targets": [
                            {"type": "Concept", "name": "human"},
                            {"type": "Concept", "name": "monkey"},
                        ],
                        "type": "",
                    }
                )
            )

    def test_add_link_without_targets_parameter(self, database: InMemoryDB):
        with pytest.raises(AddLinkException, match="'type' and 'targets' are required."):
            database.add_link(dict_to_link_params({"targets": [], "type": "Similarity"}))

    def test_add_nested_links(self, database: InMemoryDB):
        answer = database.get_matched_type("Evaluation")
        assert len(answer) == 0

        database.add_link(
            dict_to_link_params(
                {
                    "type": "Evaluation",
                    "targets": [
                        {"type": "Predicate", "name": "Predicate:has_name"},
                        {
                            "type": "Evaluation",
                            "targets": [
                                {
                                    "type": "Predicate",
                                    "name": "Predicate:has_name",
                                },
                                {
                                    "targets": [
                                        {
                                            "type": "Reactome",
                                            "name": "Reactome:R-HSA-164843",
                                        },
                                        {
                                            "type": "Concept",
                                            "name": "Concept:2-LTR circle formation",
                                        },
                                    ],
                                    "type": "Set",
                                },
                            ],
                        },
                    ],
                }
            )
        )
        answer = database.get_matched_type("Evaluation")
        assert len(answer) == 2

    def test_get_link_type(self, database: InMemoryDB):
        human = database.get_node_handle("Concept", "human")
        chimp = database.get_node_handle("Concept", "chimp")
        link_handle = database.get_link_handle(
            link_type="Similarity", target_handles=[human, chimp]
        )
        ret = database.get_link_type(link_handle=link_handle)
        assert ret == "Similarity"

    @pytest.mark.skip("Removed from C++ implementation")
    def test_build_targets_list(self, database: InMemoryDB):
        targets = database._build_targets_list(
            {
                "blah": "h0",
            }
        )
        assert targets == []
        targets = database._build_targets_list(
            {
                "key_0": "h0",
            }
        )
        assert targets == ["h0"]
        targets = database._build_targets_list(
            {
                "key_0": "h0",
                "key_1": "h1",
            }
        )
        assert targets == ["h0", "h1"]

    def test_get_atom(self, database: InMemoryDB):
        h = database.get_node_handle("Concept", "human")
        m = database.get_node_handle("Concept", "monkey")
        s = database.get_link_handle("Similarity", [h, m])
        atom = database.get_atom(handle=s)
        assert atom.handle == s
        assert atom.targets == [h, m]

        with pytest.raises(AtomDoesNotExist) as exc:
            database.get_atom(handle="test")
        assert "Nonexistent atom" in str(exc.value)
        assert "handle: test" in str(exc.value)

    def test_get_atom_as_dict(self, database: InMemoryDB):
        h = database.get_node_handle("Concept", "human")
        m = database.get_node_handle("Concept", "monkey")
        s = database.get_link_handle("Similarity", [h, m])
        atom = database.get_atom(handle=s).to_dict()
        assert atom["handle"] == s
        assert atom["targets"] == [h, m]

    def test_get_incoming_links(self, database: InMemoryDB):
        h = database.get_node_handle("Concept", "human")
        m = database.get_node_handle("Concept", "monkey")
        s = database.get_link_handle("Similarity", [h, m])

        links = database.get_incoming_links_atoms(atom_handle=h)
        atom = database.get_atom(handle=s)
        assert atom in links

        links = database.get_incoming_links_atoms(atom_handle=h, targets_document=True)
        for link in links:
            for a, b in zip(link.targets, link.targets_documents):
                assert a == b.handle

        links = database.get_incoming_links_handles(atom_handle=h)
        assert s in links

        links = database.get_incoming_links_handles(atom_handle=s)
        assert links == []

    def test_get_atom_type(self, database: InMemoryDB):
        h = database.get_node_handle("Concept", "human")
        m = database.get_node_handle("Concept", "mammal")
        i = database.get_link_handle("Inheritance", [h, m])

        assert "Concept" == database.get_atom_type(h)
        assert "Concept" == database.get_atom_type(m)
        assert "Inheritance" == database.get_atom_type(i)

    def test_get_all_links(self, database: InMemoryDB):
        link_h = database.get_all_links("Similarity")
        link_i = database.get_all_links("Inheritance")
        assert len(link_h) == 14
        assert len(link_i) == 12
        assert database.get_all_links("snet") == set()

    def test_delete_atom(self):
        cat_handle = AtomDB.node_handle("Concept", "cat")
        dog_handle = AtomDB.node_handle("Concept", "dog")
        mammal_handle = AtomDB.node_handle("Concept", "mammal")
        inheritance_cat_mammal_handle = AtomDB.link_handle(
            "Inheritance", [cat_handle, mammal_handle]
        )
        inheritance_dog_mammal_handle = AtomDB.link_handle(
            "Inheritance", [dog_handle, mammal_handle]
        )

        db = InMemoryDB()

        assert db.count_atoms() == {"atom_count": 0, "node_count": 0, "link_count": 0}

        db.add_link(
            dict_to_link_params(
                {
                    "type": "Inheritance",
                    "targets": [
                        {"type": "Concept", "name": "cat"},
                        {"type": "Concept", "name": "mammal"},
                    ],
                }
            )
        )
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Inheritance",
                    "targets": [
                        {"type": "Concept", "name": "dog"},
                        {"type": "Concept", "name": "mammal"},
                    ],
                }
            )
        )

        assert db.count_atoms() == {"atom_count": 5, "node_count": 3, "link_count": 2}

        db.delete_atom(inheritance_cat_mammal_handle)
        db.delete_atom(inheritance_dog_mammal_handle)
        assert db.count_atoms() == {"atom_count": 3, "node_count": 3, "link_count": 0}

        db.add_link(
            dict_to_link_params(
                {
                    "type": "Inheritance",
                    "targets": [
                        {"type": "Concept", "name": "cat"},
                        {"type": "Concept", "name": "mammal"},
                    ],
                }
            )
        )
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Inheritance",
                    "targets": [
                        {"type": "Concept", "name": "dog"},
                        {"type": "Concept", "name": "mammal"},
                    ],
                }
            )
        )

        db.delete_atom(mammal_handle)
        assert db.count_atoms() == {"atom_count": 2, "node_count": 2, "link_count": 0}

        db.add_link(
            dict_to_link_params(
                {
                    "type": "Inheritance",
                    "targets": [
                        {"type": "Concept", "name": "cat"},
                        {"type": "Concept", "name": "mammal"},
                    ],
                }
            )
        )
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Inheritance",
                    "targets": [
                        {"type": "Concept", "name": "dog"},
                        {"type": "Concept", "name": "mammal"},
                    ],
                }
            )
        )

        db.delete_atom(cat_handle)
        assert db.count_atoms() == {"atom_count": 3, "node_count": 2, "link_count": 1}

        db.add_link(
            dict_to_link_params(
                {
                    "type": "Inheritance",
                    "targets": [
                        {"type": "Concept", "name": "cat"},
                        {"type": "Concept", "name": "mammal"},
                    ],
                }
            )
        )

        db.delete_atom(dog_handle)
        assert db.count_atoms() == {"atom_count": 3, "node_count": 2, "link_count": 1}

        db.clear_database()

        db.add_link(
            dict_to_link_params(
                {
                    "type": "Inheritance",
                    "targets": [
                        {
                            "type": "Inheritance",
                            "targets": [
                                {"type": "Concept", "name": "dog"},
                                {
                                    "type": "Inheritance",
                                    "targets": [
                                        {"type": "Concept", "name": "cat"},
                                        {"type": "Concept", "name": "mammal"},
                                    ],
                                },
                            ],
                        },
                        {"type": "Concept", "name": "mammal"},
                    ],
                }
            )
        )

        db.delete_atom(inheritance_cat_mammal_handle)
        assert db.count_atoms() == {"atom_count": 3, "node_count": 3, "link_count": 0}

    def test_add_link_that_already_exists(self):
        db = InMemoryDB()
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Similarity",
                    "targets": [
                        {"type": "Test", "name": "test_1"},
                        {"type": "Test", "name": "test_2"},
                    ],
                }
            )
        )
        assert db.count_atoms() == {"atom_count": 3, "node_count": 2, "link_count": 1}

        db.add_link(
            dict_to_link_params(
                {
                    "type": "Similarity",
                    "targets": [
                        {"type": "Test", "name": "test_1"},
                        {"type": "Test", "name": "test_2"},
                    ],
                }
            )
        )
        assert db.count_atoms() == {"atom_count": 3, "node_count": 2, "link_count": 1}

    def test_bulk_insert(self):
        db = InMemoryDB()

        assert db.count_atoms() == {"atom_count": 0, "node_count": 0, "link_count": 0}

        documents = [
            NodeT(
                _id="node1",
                handle="node1",
                composite_type_hash="ConceptHash",
                name="human",
                named_type="Concept",
            ),
            NodeT(
                _id="node2",
                handle="node2",
                composite_type_hash="ConceptHash",
                name="monkey",
                named_type="Concept",
            ),
            LinkT(
                _id="link1",
                handle="link1",
                composite_type_hash="CompositeTypeHash",
                is_toplevel=True,
                composite_type=["SimilarityHash", "ConceptHash", "ConceptHash"],
                named_type="Similarity",
                named_type_hash="SimilarityHash",
                targets=["node1", "node2"],
            ),
        ]

        db.bulk_insert(documents)

        assert db.count_atoms() == {"atom_count": 3, "node_count": 2, "link_count": 1}

    def test_retrieve_all_atoms(self, database: InMemoryDB):
        expected = self.all_added_nodes + self.all_added_links
        assert len(expected) == len(self.all_added_nodes + self.all_added_links)
        actual = database.retrieve_all_atoms()
        assert len(expected) == len(actual)
        assert sorted([e.handle for e in expected]) == sorted([a.handle for a in actual])
        assert sorted([e.to_dict() for e in expected], key=lambda d: d["handle"]) == sorted(
            [a.to_dict() for a in actual], key=lambda d: d["handle"]
        )
        assert len(expected) == len(set([e.handle for e in expected]))
        assert sorted([e.handle for e in expected]) == sorted(
            list(set([e.handle for e in expected]))
        )
        assert len(actual) == len(set([a.handle for a in actual]))
        assert sorted([a.handle for a in actual]) == sorted(list(set([a.handle for a in actual])))
