import functools
from unittest import mock

import pytest

from hyperon_das_atomdb.database import AtomDB, AtomT, LinkT, NodeT
from tests.python.helpers import add_link, add_node, check_handle, dict_to_link_params, dict_to_node_params
from tests.python.unit.fixtures import in_memory_db, redis_mongo_db  # noqa: F401


def pytest_generate_tests(metafunc):
    idlist = []
    argvalues = []
    for scenario in metafunc.cls.scenarios:
        idlist.append(scenario[0])
        items = scenario[1].items()
        argnames = [x[0] for x in items]
        argvalues.append([x[1] for x in items])
    metafunc.parametrize(argnames, argvalues, ids=idlist, scope="class")


in_memory = ("memory", {"database": "in_memory_db"})
redis_mongo = ("redis_mongo", {"database": "redis_mongo_db"})


class TestDatabase:
    scenarios = [in_memory, redis_mongo]

    def _load_db(self, db):
        import json
        import pathlib

        path = pathlib.Path(__file__).parent.resolve()
        with open(f"{path}/adapters/data/ram_only_nodes.json") as f:
            all_nodes = json.load(f)
        with open(f"{path}/adapters/data/ram_only_links.json") as f:
            all_links = json.load(f)
        for node in all_nodes:
            db.add_node(dict_to_node_params(node))
        for link in all_links:
            db.add_link(dict_to_link_params(link))

    def _load_db_redis_mongo(self, db):
        import json
        import pathlib

        path = pathlib.Path(__file__).parent.resolve()
        with open(f"{path}/adapters/data/atom_mongo_redis.json") as f:
            atoms = json.load(f)
            for atom in atoms:
                if "name" in atom:
                    db.add_node(dict_to_node_params(atom))
                else:
                    # atom.update({"named_type": atom["type"]})
                    top_level = atom["is_toplevel"]
                    del atom["is_toplevel"]
                    db.add_link(dict_to_link_params(atom), toplevel=top_level)
        try:
            db.commit()
        except:  # noqa: F841,E722
            pass

    @pytest.mark.parametrize(
        "expected",
        [
            ("1fd600f0fd8a1fab79546a4fc3612df3"),  # Concept, Human
            ("1fd600f0fd8a1fab79546a4fc3612df3"),  # Concept, Human
        ],
    )
    def test_node_handle(self, database, expected, request):
        db: AtomDB = request.getfixturevalue(database)
        handle = db.node_handle("Concept", "Human")
        incorrect_handle = db.node_handle("Concept", "human")
        assert handle, incorrect_handle
        assert handle != incorrect_handle
        assert handle == expected
        assert check_handle(handle)

    @pytest.mark.parametrize(
        "expected",
        [
            ("1fd600f0fd8a1fab79546a4fc3612df3"),  # Concept, Human
            ("1fd600f0fd8a1fab79546a4fc3612df3"),  # Concept, Human
        ],
    )
    def test_node_handle_exceptions(self, database, expected, request):
        db: AtomDB = request.getfixturevalue(database)
        # NOTE Should raise ValueError
        with pytest.raises(TypeError):
            db.node_handle([], [])

    @pytest.mark.parametrize(
        "expected",
        [
            ("a9dea78180588431ec64d6bc4872fdbc"),  # Similarity
            ("a9dea78180588431ec64d6bc4872fdbc"),  # Similarity
        ],
    )
    def test_link_handle(self, database, expected, request):
        db: AtomDB = request.getfixturevalue(database)
        handle = db.link_handle("Similarity", [])
        handles = set([db.link_handle("Similarity", f) for f in [[], [], (), list(), tuple()]])
        assert len(handles) == 1, handles
        assert handle
        assert check_handle(handle)

    @pytest.mark.parametrize(
        "expected",
        [
            ("a9dea78180588431ec64d6bc4872fdbc"),  # Similarity
            ("a9dea78180588431ec64d6bc4872fdbc"),  # Similarity
        ],
    )
    def test_link_handle_exceptions(self, database, expected, request):
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(TypeError):
            db.link_handle("Similarity", None)
        # NOTE Unreachable
        # TODO: unreachable code must be deleted or fixed to become reachable
        # with pytest.raises(ValueError):
        #     db.link_handle("Similarity", set())

    def test_node_exists(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        db.add_node(NodeT(name="A", type="Test"))
        if database != "in_memory_db":
            db.commit()
        no_exists = db.node_exists("Test", "B")
        exists = db.node_exists("Test", "A")
        assert isinstance(no_exists, bool)
        assert isinstance(exists, bool)
        assert not no_exists
        assert exists

    @pytest.mark.parametrize(
        "targets",
        [
            (["180fed764dbd593f1ea45b63b13d7e69"]),
        ],
    )
    def test_link_exists(self, database, targets, request):
        db: AtomDB = request.getfixturevalue(database)
        targets_params = [NodeT(type="Test", name="test")]
        link = LinkT("Test", targets_params)
        db.add_link(link)
        if database != "in_memory_db":
            db.commit()
        no_exists = db.link_exists("Tes", [])
        exists = db.link_exists("Test", targets)

        assert isinstance(no_exists, bool)
        assert isinstance(exists, bool)
        assert not no_exists
        assert exists

    def test_get_node_handle(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        expected_node = add_node(db, "A", "Test", database)
        node = db.get_node_handle("Test", "A")
        assert node == expected_node.handle
        assert check_handle(node)

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
    def test_get_node_handle_loaded(self, database, node_type, node_name, expected, request):
        db = request.getfixturevalue(database)
        self._load_db(db)
        if database == "redis_mongo_db":
            db.commit()
        actual = db.get_node_handle(node_type=node_type, node_name=node_name)
        assert expected == actual
        assert check_handle(actual)

    def test_get_node_handle_exceptions(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(Exception, match="Nonexistent atom"):
            db.get_node_handle("#$QQ!#", "A")

    def test_get_node_name(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        expected_node = add_node(db, "A", "Test", database)
        name = db.get_node_name(expected_node.handle)
        # NOTE all adapters must return the same type
        assert isinstance(name, str)
        assert name == expected_node.name

    def test_get_node_name_exceptions(self, database, request):
        if database == "redis_mongo_db":
            # TODO: fix this
            pytest.skip(
                "ERROR in_memory returns a AtomDoesNotExist exception, redis_mongo returns ValueError. "
                "See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        # in memory returns a AtomDoesNotExist exception, redis_mongo returns ValueError
        # TODO: should this be fixed/synced? I mean, make both raise the same exception?
        with pytest.raises(Exception, match="Nonexistent atom"):
            db.get_node_name("error")

    def test_get_node_type(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        expected_node = add_node(db, "A", "Test", database)
        node_type = db.get_node_type(expected_node.handle)
        assert isinstance(node_type, str)
        assert node_type == expected_node.named_type

    def test_get_node_type_exceptions(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(Exception, match="Nonexistent atom"):
            db.get_node_type("test")

    def test_get_node_by_name(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        expected_nodes = [add_node(db, n, "Test", database) for n in {"A", "Aa", "Ac"}]
        not_expected_nodes = [add_node(db, n, "Test", database) for n in {"B", "Ba", "Bc"}]
        nodes = db.get_node_by_name("Test", "A")
        not_nodes = db.get_node_by_name("Test", "C")
        assert not_nodes == []
        assert isinstance(nodes, list)
        assert len(nodes) == 3
        assert all(check_handle(node) for node in nodes)
        assert all(n.handle in nodes for n in expected_nodes)
        assert not any(n.handle in nodes for n in not_expected_nodes)

    @pytest.mark.parametrize(
        "atom_type,atom_values,query_values,expected",
        [
            (
                "node",  # atom_type
                {"node_name": "Ac", "node_type": "Test"},  # atom_values
                [{"field": "name", "value": "Ac"}],  # query_values
                "785a4a9c6a986f8b1ba35d0de70e8fd8",  # expected
            ),
            (
                "link",  # atom_type
                {"link_type": "Ac", "dict_targets": [NodeT("A", "A")]},  # atom_values
                [{"field": "named_type", "value": "Ac"}],  # query_values
                "8ec320f9ffe82c28fcefd256a20b5c60",  # expected
            ),
        ],
    )
    def test_get_atoms_by_field(
        self, database, atom_type, atom_values, query_values, expected, request
    ):
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        if atom_type == "link":
            add_link(
                db,
                link_type=atom_values["link_type"],
                targets=atom_values["dict_targets"],
                adapter=database,
            )
        else:
            add_node(
                db,
                node_name=atom_values["node_name"],
                node_type=atom_values["node_type"],
                adapter=database,
            )
        atoms = db.get_atoms_by_field(query_values)
        assert isinstance(atoms, list)
        assert all(check_handle(atom) for atom in atoms)
        assert atoms[0] == expected

    @pytest.mark.parametrize(
        "index_params,query_params,expected",
        [
            (
                # index_params
                {"atom_type": "node", "fields": ["value"], "named_type": "Test"},
                # query_params / custom attributes
                [{"field": "value", "value": 3}],
                # expected
                "815212e3d7ac246e70c1744d14a8c402",
            ),
            (
                {"atom_type": "node", "fields": ["value", "strength"], "named_type": "Test"},
                [{"field": "value", "value": 3}, {"field": "strength", "value": 5}],
                "815212e3d7ac246e70c1744d14a8c402",
            ),
            (
                {"atom_type": "link", "fields": ["value"], "named_type": "Test3"},
                [{"field": "value", "value": 3}],
                "b3f66ec1535de7702c38e94408fa4a17",
            ),
            (
                {"atom_type": "link", "fields": ["value"], "named_type": "Test2"},
                [{"field": "value", "value": 3}, {"field": "round", "value": 2}],
                "c454552d52d55d3ef56408742887362b",
            ),
            (
                {"atom_type": "link", "fields": ["value", "round"], "named_type": "Test"},
                [{"field": "value", "value": 3}, {"field": "round", "value": 2}],
                "0cbc6611f5540bd0809a388dc95a615b",
            ),
        ],
    )
    def test_get_atoms_by_index(self, database, index_params, query_params, expected, request):
        pytest.skip(
            "Requires new implementation since the new custom attributes were introduced. See https://github.com/singnet/das-atom-db/issues/255"
        )
        if database == "in_memory_db":
            pytest.skip(
                "ERROR Not implemented. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        if index_params.get("atom_type") == "link":
            atom = add_link(
                db,
                index_params["named_type"],
                [],
                database,
                extra_fields={k["field"]: k["value"] for k in query_params},
            )
        else:
            atom = add_node(
                db,
                "A",
                index_params["named_type"],
                database,
                {k["field"]: k["value"] for k in query_params},
            )
        index_id = db.create_field_index(
            atom_type=index_params.get("atom_type"),
            fields=[k["field"] for k in query_params],
            named_type=index_params["named_type"],
        )
        cursor, atoms = db.get_atoms_by_index(index_id, query_params)
        assert isinstance(cursor, int)
        assert isinstance(atoms, list)
        assert cursor == 0
        assert len(atoms) == 1
        handles = [atom.handle for atom in atoms]
        assert atom.handle in handles
        assert expected in handles
        assert all(isinstance(a, AtomT) for a in atoms)

    def test_get_atoms_by_index_exceptions(self, database, request):
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(Exception):
            db.get_atoms_by_index("", [])

    def test_get_atoms_by_text_field_regex(self, database, request):
        pytest.skip(
            "Requires new implementation since the new custom attributes were introduced. See https://github.com/singnet/das-atom-db/issues/255"
        )
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        add_node(db, "A", "Test", database, {"value": "Test sentence"})
        add_link(db, "Test", [], database)
        index_id = db.create_field_index(
            atom_type="node", fields=["value"], named_type="Test", index_type="token_inverted_list"
        )
        atoms = db.get_atoms_by_text_field("Test", "value", text_index_id=index_id)
        assert isinstance(atoms, list)
        assert all(check_handle(a) for a in atoms)
        assert all(isinstance(a, str) for a in atoms)
        assert len(atoms) == 1

    def test_get_atoms_by_text_field_text(self, database, request):
        pytest.skip(
            "Requires new implementation since the new custom attributes were introduced. See https://github.com/singnet/das-atom-db/issues/255"
        )
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        add_node(db, "A", "Test", database, {"value": "Test sentence"})
        index_id = db.create_field_index(
            atom_type="node", fields=["value"], named_type="Test", index_type="token_inverted_list"
        )
        with mock.patch(
            "mongomock.filtering._Filterer.apply", return_value=["815212e3d7ac246e70c1744d14a8c402"]
        ):
            atoms = db.get_atoms_by_text_field("Test", text_index_id=index_id)
            assert isinstance(atoms, list)
            assert all(check_handle(a) for a in atoms)
            assert all(isinstance(a, str) for a in atoms)
            assert len(atoms) == 1

    def test_get_node_by_name_starting_with(self, database, request):
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        node_b = add_node(db, "Abb", "Test", database)
        add_node(db, "Bbb", "Test", database)
        nodes = db.get_node_by_name_starting_with("Test", "A")
        assert isinstance(nodes, list)
        assert all(check_handle(n) for n in nodes)
        assert all(isinstance(n, str) for n in nodes)
        assert all(handle in nodes for handle in [node_a.handle, node_b.handle])
        assert len(nodes) == 2

    @pytest.mark.parametrize(
        "params,nodes_len",
        [  # TODO: differences here must be fixed if possible
            ({"node_type": "Test"}, 3),
            ({"node_type": "Test", "names": True}, 3),
            ({"node_type": "Z", "names": True}, 0),
            ({"node_type": "Test2"}, 2),
            ({"node_type": "Test2", "names": True}, 2),
        ],
    )
    def test_get_all_nodes(self, database, params, nodes_len, request):
        db: AtomDB = request.getfixturevalue(database)
        values = {"Test": ["Aaa", "Abb", "Bbb"], "Test2": ["Bcc", "Ccc"]}
        _ = [
            add_node(db, node_name, node_type, database)
            for node_type, node_names in values.items()
            for node_name in node_names
        ]
        names: bool = params.pop("names", False)
        if names:
            nodes = db.get_all_nodes_names(**params)
        else:
            nodes = db.get_all_nodes_handles(**params)
        assert isinstance(nodes, list)
        if not names:
            assert all(check_handle(n) for n in nodes)
        assert all(isinstance(n, str) for n in nodes)
        assert len(nodes) == nodes_len

    @pytest.mark.parametrize(
        "params,links_len",
        [  # TODO: differences here must be fixed if possible
            ({"link_type": "Ac"}, 3),
            ({"link_type": "Ac", "names": True}, 3),
            ({"link_type": "Z", "names": True}, 0),
            ({"link_type": "Ac"}, 3),
            # NOTE should return the same value for the cursor
            ({"link_type": "Ac", "names": True}, 3),
            ({"link_type": "Z", "names": True}, 0),
        ],
    )
    def test_get_all_links(self, database, params, links_len, request):
        db: AtomDB = request.getfixturevalue(database)
        add_link_ = functools.partial(add_link, link_type="Ac", db=db, adapter=database)
        for node_name, node_type in (("A", "A"), ("B", "B"), ("C", "C")):
            add_link_(targets=[NodeT(name=node_name, type=node_type)])
        links = db.get_all_links(**params)
        assert isinstance(links, set)
        assert all(check_handle(link) for link in links)
        assert all(isinstance(link, str) for link in links)
        assert len(links) == links_len

    def test_get_link_handle(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        link = add_link(db, "Ac", [NodeT(name="A", type="A")], database)
        handle = db.get_link_handle(link.named_type, link.targets)
        assert check_handle(handle)

    def test_get_link_handle_exceptions(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(Exception, match="Nonexistent atom"):
            db.get_link_handle("A", [])

    def test_get_link_type(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        node_params = NodeT(name="A", type="A")
        link_a = add_link(db, "Ac", [node_params], database)
        add_link(db, "Bc", [node_params], database)
        link_type = db.get_link_type(link_a.handle)
        assert link_type
        assert isinstance(link_type, str)
        assert link_type == link_a.named_type

    def test_get_link_type_exceptions(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(Exception, match="Nonexistent atom"):
            db.get_link_type("")

    def test_get_link_targets(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        link_a = add_link(db, "Ac", [NodeT(name="A", type="A")], database)
        targets = db.get_link_targets(link_a.handle)
        assert isinstance(targets, list)
        assert len(targets) == 1
        assert all(check_handle(t) for t in targets)
        assert all(isinstance(t, str) for t in targets)
        assert targets == link_a.targets

    @pytest.mark.parametrize(
        "params,links_len",
        [  # TODO: differences here must be fixed if possible
            ({}, 3),
            ({"handles_only": True}, 3),
            ({"handles_only": False}, 3),
            ({"no_target_format": True}, 3),
        ],
    )
    def test_get_incoming_links(self, database, params, links_len, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        add_link(db, "Aa", [node_a], database)
        add_link(db, "Ab", [node_a], database)
        add_link(db, "Ac", [node_a], database)
        if params.get("handles_only"):
            links = db.get_incoming_links_handles(node_a.handle, **params)
        else:
            links = db.get_incoming_links_atoms(node_a.handle, **params)
        assert len(links) == links_len
        assert all(
            [check_handle(link if params.get("handles_only") else link.handle) for link in links]
        )

    @pytest.mark.parametrize(
        "params,links_len",
        [  # TODO: differences here must be fixed if possible
            ({}, 1),
            ({"toplevel_only": True}, 1),
            # ("redis_mongo_db", {"link_type": "NoTopLevel" , "toplevel_only": True}, 0), # doesn"t work
            # Note returning different values
            # ("in_memory_db", {"link_type": "*", "toplevel_only": True}, 3),
            # ("redis_mongo_db", {"link_type": "*", "toplevel_only": True}, 0),
            # ("redis_mongo_db", {"link_type": "*"}, 3), # should return 3
            ({"target_handles": ["*"]}, 1),
            ({"handles_only": True}, 1),
            ({"no_target_format": True}, 1),
            ({}, 1),
            ({"toplevel_only": True}, 1),
            # ("in_memory_db", {"link_type": "NoTopLevel", "toplevel_only": True}, 0), # doesn"t work
            # ( {"link_type": "*"}, 3),
            ({"target_handles": ["*"]}, 1),
            ({"handles_only": True}, 1),
            ({"no_target_format": True}, 1),
        ],
    )
    def test_get_matched_links(self, database, params, links_len, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        link_a = add_link(db, "Aa", [node_a], database)
        add_link(db, "NoTopLevel", [node_a], database, is_toplevel=False)
        add_link(db, "Ac", [node_a], database)
        if not params.get("link_type"):
            params["link_type"] = link_a.named_type
        if not params.get("target_handles"):
            params["target_handles"] = link_a.targets
        links = db.get_matched_links(**params)
        assert len(links) == links_len
        if all(isinstance(link, tuple) for link in links):
            for link in links:
                while link:
                    assert check_handle(link[0])
                    link = link[1] if len(link) > 1 else None
        else:
            assert all([check_handle(link) for link in links])

    @pytest.mark.parametrize(
        "params,links_len",
        [
            ({"link_type": "*", "target_handles": ["*", "*"]}, 2),
            ({"link_type": "*", "target_handles": ["*"]}, 1),
            ({"link_type": "Aa", "target_handles": ["123123"]}, 0),
            ({"link_type": "Aa", "target_handles": ["afdb1c23e7f2da1f33c2a3a91d7959a7"]}, 1),
            ({"link_type": "*", "target_handles": ["afdb1c23e7f2da1f33c2a3a91d7959a7"]}, 1),
            (
                {
                    "link_type": "Bab",
                    "target_handles": [
                        "afdb1c23e7f2da1f33c2a3a91d7959a7",
                        "762745ca7757082780f428ba4116ea46",
                    ],
                },
                1,
            ),
            (
                {
                    "link_type": "*",
                    "target_handles": [
                        "afdb1c23e7f2da1f33c2a3a91d7959a7",
                        "762745ca7757082780f428ba4116ea46",
                    ],
                },
                1,
            ),
            ({"link_type": "*", "target_handles": ["afdb1c23e7f2da1f33c2a3a91d7959a7", "*"]}, 2),
            ({"link_type": "*", "target_handles": ["*", "762745ca7757082780f428ba4116ea46"]}, 1),
            ({"link_type": "*", "target_handles": ["*", "47a0059c63c6943615c232a29a315018"]}, 1),
            ({"link_type": "CaA", "target_handles": ["*", "47a0059c63c6943615c232a29a315018"]}, 1),
        ],
    )
    def test_get_matched_links_more(self, database, params, links_len, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a_d = {"name": "a", "type": "Test"}
        node_b_d = {"name": "b", "type": "Test"}
        link_a_d = {"type": "Aa", "targets": [node_a_d]}
        # afdb1c23e7f2da1f33c2a3a91d7959a7
        add_node(db, "a", "Test", database)
        # 762745ca7757082780f428ba4116ea46
        add_node(db, "b", "Test", database)
        # 47a0059c63c6943615c232a29a315018
        add_link(db, "Aa", [dict_to_node_params(node_a_d)], database)
        # 51255240d91ea1e045260355cf19d3b2
        add_link(
            db, "Bab", [dict_to_node_params(node_a_d), dict_to_node_params(node_b_d)], database
        )
        # 2b9c92b0b219881f4b6121f08f4850ba
        add_link(
            db, "CaA", [dict_to_node_params(node_a_d), dict_to_link_params(link_a_d)], database
        )
        links = db.get_matched_links(**params)
        assert len(links) == links_len

    @pytest.mark.parametrize(
        "link_type,link_targets,expected_count",
        [
            ("*", ["*", "*"], 28),
            # ("LinkTest", ["*", "*", "*", "*"], 1),
            ("Similarity", ["*", "af12f10f9ae2002a1607ba0b47ba8407"], 3),
            ("Similarity", ["af12f10f9ae2002a1607ba0b47ba8407", "*"], 3),
            (
                "Inheritance",
                ["c1db9b517073e51eb7ef6fed608ec204", "b99ae727c787f1b13b452fd4c9ce1b9a"],
                1,
            ),
            (
                "Evaluation",
                ["d03e59654221c1e8fcda404fd5c8d6cb", "99d18c702e813b07260baf577c60c455"],
                1,
            ),
            (
                "Evaluation",
                ["d03e59654221c1e8fcda404fd5c8d6cb", "99d18c702e813b07260baf577c60c455"],
                1,
            ),
            ("Evaluation", ["*", "99d18c702e813b07260baf577c60c455"], 1),
        ],
    )
    def test_patterns(self, link_type, link_targets, expected_count, database, request):
        db: AtomDB = request.getfixturevalue(database)
        self._load_db_redis_mongo(db)
        links = db.get_matched_links(link_type, link_targets)
        assert len(links) == expected_count

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
    def test_get_matched_links_loaded(self, database, targets, link_type, expected, request):
        db: AtomDB = request.getfixturevalue(database)
        self._load_db(db)
        if database == "redis_mongo_db":
            db.commit()
        target_handles = [db.get_node_handle(*t) if t != "*" else t for t in targets]
        actual = db.get_matched_links(link_type, target_handles)
        assert actual
        assert isinstance(actual, set)
        assert expected == actual

    @pytest.mark.parametrize(
        "params,links_len",
        [  # TODO: differences here must be fixed if possible
            ({"link_type": "Z", "target_handles": []}, 0),
            # ("redis_mongo_db", {"link_type": "*", "target_handles": ["*", "*"], "toplevel_only": True}, 0),
            # ("in_memory_db", {"link_type": "*", "target_handles": ["*", "*"]}, 0),
            # ("in_memory_db", {"link_type": "*", "target_handles": ["*", "*"], "toplevel_only": True}, 0),
        ],
    )
    def test_get_matched_no_links(self, database, params, links_len, request):
        db: AtomDB = request.getfixturevalue(database)
        add_node(db, "Aaa", "Test", database)
        links = db.get_matched_links(**params)
        assert len(links) == links_len

    @pytest.mark.parametrize(
        "params,links_len,is_toplevel",
        [  # TODO: differences here must be fixed if possible
            ({}, 1, True),
            ({}, 1, False),
            ({"toplevel_only": True}, 0, False),
            # NOTE should return None or same as redis_mongo
        ],
    )
    def test_get_matched_type_template(self, database, params, links_len, is_toplevel, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        node_b = add_node(db, "Bbb", "Test", database)
        link_a = add_link(db, "Aa", [node_a, node_b], database, is_toplevel=is_toplevel)
        links = db.get_matched_type_template(["Aa", *["Test", "Test"]], **params)
        assert len(links) == links_len
        if len(links) > 0:
            for link in links:
                assert check_handle(link)
                assert link == link_a.handle
                assert sorted(db.get_atom(link).targets) == sorted(link_a.targets)

    def test_get_matched_type(self, database, request):
        if database == "redis_mongo_db":
            # TODO: fix this
            pytest.skip(
                "ERROR redis_mongo_db is returning more values. "
                "See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        targets_params = [NodeT(type="Test", name="test")]
        link_a = add_link(db, "Aa", targets_params, database)
        add_link(db, "Ab", targets_params, database)
        links = db.get_matched_type(link_a.named_type)
        assert len(links) == 1
        if len(links) > 0:
            for link in links:
                assert check_handle(link)
                assert link == link_a.handle
                assert sorted(db.get_atom(link).targets) == sorted(link_a.targets)

    @pytest.mark.parametrize(
        "params,top_level,n_links,n_nodes",
        [  # TODO: differences here must be fixed if possible
            ({}, True, 1, 1),
            ({"no_target_format": True}, False, 1, 1),
            ({"no_target_format": False}, False, 1, 1),
            ({"targets_document": True}, False, 1, 1),
            ({"deep_representation": True}, False, 1, 1),
            ({"deep_representation": False}, False, 1, 1),
        ],
    )
    def test_get_atom_node(self, database, params, top_level, n_links, n_nodes, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        atom_n = db.get_atom(node_a.handle, **params)
        assert atom_n
        assert atom_n.handle == node_a.handle
        assert check_handle(atom_n.handle)

    @pytest.mark.parametrize(
        "params,top_level,n_links,n_nodes",
        [
            ({}, True, 1, 1),
            ({"no_target_format": True}, False, 1, 1),
            ({"targets_document": True}, False, 1, 1),
            ({"deep_representation": True}, False, 1, 1),
        ],
    )
    def test_get_atom_link(self, database, params, top_level, n_links, n_nodes, request):
        db: AtomDB = request.getfixturevalue(database)
        link_a = add_link(
            db, "Aa", [NodeT(type="Test", name="test")], database, is_toplevel=top_level
        )
        atom_l = db.get_atom(link_a.handle, **params)
        assert atom_l
        assert atom_l.handle == link_a.handle
        assert check_handle(atom_l.handle)

    def test_get_atom_type(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        link_a = add_link(db, "Test", [node_a], database)
        atom_type_node = db.get_atom_type(node_a.handle)
        atom_type_link = db.get_atom_type(link_a.handle)
        assert isinstance(atom_type_node, str)
        assert isinstance(atom_type_link, str)
        assert atom_type_node == atom_type_link

    def test_get_atom_type_none(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        atom_type_node = db.get_atom_type("handle")
        atom_type_link = db.get_atom_type("handle")
        assert atom_type_node is None
        assert atom_type_link is None

    # NOTE: not needed - Atom class has a method to get the atom as a dict (`as_dict`)
    # def test_get_atom_as_dict(self, database, request):
    #     if database == "in_memory_db":
    #         pytest.skip("in_memory_db doesn't implement this `get_atom_as_dict`")
    #     db: AtomDB = request.getfixturevalue(database)
    #     node_a = add_node(db, "Aaa", "Test", database)
    #     link_a = add_link(db, "Test", [node_a], database)
    #     atom_node = db.get_atom_as_dict(node_a.handle)
    #     atom_link = db.get_atom_as_dict(link_a.handle)
    #     assert isinstance(atom_node, dict)
    #     assert isinstance(atom_link, dict)

    # NOTE: not needed - Atom class has a method to get the atom as a dict (`as_dict`)
    # def test_get_atom_as_dict_exception(self, database, request):
    #     if database == "in_memory_db":
    #         pytest.skip("in_memory_db doesn't implement this `get_atom_as_dict`")
    #     db: AtomDB = request.getfixturevalue(database)
    #     with pytest.raises(AtomDoesNotExist, match="Nonexistent atom"):
    #         db.get_atom_as_dict("handle")

    def test_get_atom_as_dict_exceptions(self, database, request):
        if database == "in_memory_db":
            pytest.skip("in_memory_db doesn't implement this `get_atom_as_dict`")
        if database == "redis_mongo_db":
            # TODO: fix this
            pytest.skip(
                "ERROR redis_mongo_db doesn't raises exception, they should return the same result/exception. "
                "See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(Exception, match="Nonexistent atom"):
            db.get_atom_as_dict("handle")
        with pytest.raises(Exception, match="Nonexistent atom"):
            db.get_atom_as_dict("handle")

    @pytest.mark.parametrize(
        "params",
        [  # TODO: differences here must be fixed if possible
            ({}),
            ({"precise": True}),
            ({"precise": False}),
            # NOTE should return the same value if the arg precise is set
        ],
    )
    def test_count_atoms(self, database, params, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        add_link(db, "Test", [node_a], database)
        atoms_count = db.count_atoms(params)  # InMemoryDB ignores params
        assert atoms_count
        assert isinstance(atoms_count, dict)
        assert isinstance(atoms_count["atom_count"], int)
        assert atoms_count["atom_count"] == 2
        if params.get("precise", False):
            assert isinstance(atoms_count["node_count"], int)
            assert isinstance(atoms_count["link_count"], int)
            assert atoms_count["node_count"] == 1
            assert atoms_count["link_count"] == 1

    def test_clear_database(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        add_link(db, "Test", [node_a], database)
        assert db.count_atoms()["atom_count"] == 2
        db.clear_database()
        assert db.count_atoms()["atom_count"] == 0

    @pytest.mark.parametrize(
        "node",
        [
            ({"name": "A", "type": "A"}),
            ({"name": "A", "type": "A"}),
        ],
    )
    def testadd_node(self, database, node, request):
        db: AtomDB = request.getfixturevalue(database)
        if database == "redis_mongo_db":
            db.mongo_bulk_insertion_limit = 1
        node = db.add_node(NodeT(name="A", type="A"))
        count = db.count_atoms()
        assert node
        assert count["atom_count"] == 1
        assert isinstance(node, NodeT)

    @pytest.mark.parametrize(
        "node",
        [
            ({"name": "AAAA", "type": "A"}),
        ],
    )
    def test_add_node_discard(self, database, node, request):
        if database == "in_memory_db":
            pytest.skip("Doesn't work")
        db: AtomDB = request.getfixturevalue(database)
        db.mongo_bulk_insertion_limit = 1
        db.max_mongo_db_document_size = 1
        node_params = NodeT(name="AAAA", type="A")
        node = db.add_node(node_params)
        count = db.count_atoms()
        assert node is None
        assert count["atom_count"] == 0

    @pytest.mark.parametrize(
        "node",
        [  # TODO: differences here must be fixed if possible
            ({}),
            # NOTE it"s not breaking, should break?
            # ("redis_mongo_db", {"name": "A", "type": "A", "handle": ""}),
            # ("redis_mongo_db", {"name": "A", "type": "A", "_id": ""}),
            # ("redis_mongo_db", {"name": "A", "type": "A", "composite_type_hash": ""}),
            # ("redis_mongo_db", {"name": "A", "type": "A", "named_type": ""}),
            # NOTE it"s not breaking, should break?
            # ("in_memory_db", {"name": "A", "type": "A", "handle": ""}),
            # ("in_memory_db", {"name": "A", "type": "A", "_id": ""}),
            # ("in_memory_db", {"name": "A", "type": "A", "composite_type_hash": ""}),
            # ("in_memory_db", {"name": "A", "type": "A", "named_type": ""}),
        ],
    )
    def testadd_node_exceptions(self, database, node, request):
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(Exception):
            db.add_node(node)

    @pytest.mark.parametrize(
        "params,expected_count,top_level",
        [
            ({"type": "A", "targets": [{"name": "A", "type": "A"}]}, 2, True),
        ],
    )
    def testadd_link(self, database, params, expected_count, top_level, request):
        db: AtomDB = request.getfixturevalue(database)
        if database == "redis_mongo_db":
            db.mongo_bulk_insertion_limit = 1
        targets = [dict_to_node_params(t) for t in params["targets"]]
        link = db.add_link(
            LinkT(type=params["type"], targets=targets),
            top_level,
        )
        count = db.count_atoms()
        assert link
        assert count["atom_count"] == expected_count
        assert isinstance(link, LinkT)

    @pytest.mark.parametrize(
        "params",
        [
            ({"pattern_index_templates": {}}),
        ],
    )
    def test_reindex(self, database, params, request):
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        add_link(db, "Test", [node_a], database)
        db.reindex(params)

    def test_delete_atom(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        node_b = add_node(db, "Bbb", "Test", database)
        link_a = add_link(db, "Test", [node_b], database)
        count = db.count_atoms({"precise": True})
        assert count["atom_count"] == 3
        assert count["node_count"] == 2
        assert count["link_count"] == 1
        db.delete_atom(node_a.handle)
        count = db.count_atoms({"precise": True})
        assert count["atom_count"] == 2
        assert count["node_count"] == 1
        assert count["link_count"] == 1
        db.delete_atom(link_a.handle)
        count = db.count_atoms({"precise": True})
        assert count["atom_count"] == 1
        assert count["node_count"] == 1
        assert count["link_count"] == 0

    def test_delete_atom_exceptions(self, database, request):
        if database == "in_memory_db":
            # TODO: fix this
            # TODO: C++ implementation does not raise any exception when atom does not exist
            pytest.skip(
                "ERROR Atom not in incoming_set. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(Exception):
            db.delete_atom("handle")

    @pytest.mark.parametrize(
        "params",
        [
            {"atom_type": "A", "fields": ["value"]},
            {"atom_type": "A", "fields": ["value"], "named_type": "A"},
            {"atom_type": "A", "fields": ["value"], "composite_type": ["value", "side"]},
            {
                "atom_type": "A",
                "fields": ["value"],
                "named_type": "A",
                "index_type": "binary_tree",
            },
            {
                "atom_type": "A",
                "fields": ["value", "side"],
                "composite_type": ["value", "side"],
                "index_type": "binary_tree",
            },
            {
                "atom_type": "A",
                "fields": ["value", "side"],
                "composite_type": ["value", "side"],
                "index_type": "token_inverted_list",
            },
            {
                "atom_type": "A",
                "fields": ["value"],
                "named_type": "A",
                "index_type": "token_inverted_list",
            },
            {
                "atom_type": "A",
                "fields": ["value"],
                "named_type": "A",
                "index_type": "binary_tree",
            },
            {
                "atom_type": "A",
                "fields": ["value"],
                "composite_type": ["value", "side"],
                "index_type": "token_inverted_list",
            },
            {
                "atom_type": "A",
                "fields": ["value"],
                "composite_type": ["value", "side"],
                "index_type": "binary_tree",
            },
        ],
    )
    def test_create_field_index(self, database, params, request):
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented on in_memory_db. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        index_id = db.create_field_index(**params)
        assert index_id
        assert isinstance(index_id, str)
        # checking if creating a duplicated index breaks
        index_id2 = db.create_field_index(**params)
        assert isinstance(index_id2, str)
        assert index_id2 == index_id

    @pytest.mark.parametrize(
        "params",
        [
            {"atom_type": "A", "fields": []},
            {
                "atom_type": "A",
                "fields": ["value"],
                "named_type": "A",
                "composite_type": ["value", "side"],
            },
        ],
    )
    def test_create_field_index_value_error(self, database, params, request):
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented on in_memory_db. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(ValueError):
            db.create_field_index(**params)

    # TODO: fix this or delete
    # @pytest.mark.parametrize("database,params", [
    #     ("redis_mongo_db", {"atom_type": "A", "fields": ["side"], "index_type": "wrong_type"}),
    #     ("in_memory_db", {})
    # ])
    # def test_create_field_index_mongo_error(self, database, params, request):
    #     if database == "in_memory_db":
    #         # TODO: fix this
    #         pytest.skip("ERROR Not implemented on in_memory_db. See https://github.com/singnet/das-atom-db/issues/210")
    #
    #     db: AtomDB = request.getfixturevalue(database)
    #     # with pytest.raises(ValueError):
    #     db.create_field_index(**params)

    def test_bulk_insert(self, database, request):
        if database == "redis_mongo_db":
            # TODO: fix this
            pytest.skip(
                "ERROR redis_mongo_db is not updating targets. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        link_a = add_link(
            db,
            "Test",
            [NodeT(name="A", type="A")],
            database,
        )
        node_a_copy = node_a.__class__(
            name="B",  # different name
            _id=node_a._id,
            handle=node_a.handle,
            composite_type_hash=node_a.composite_type_hash,
            named_type=node_a.named_type,
        )
        link_a_copy = link_a.__class__(
            targets=[node_a_copy.handle],  # different targets
            _id=link_a._id,
            handle=link_a.handle,
            composite_type_hash=link_a.composite_type_hash,
            named_type=link_a.named_type,
            composite_type=link_a.composite_type,
            named_type_hash=link_a.named_type_hash,
            is_toplevel=link_a.is_toplevel,
        )
        db.bulk_insert([node_a_copy, link_a_copy])
        count = db.count_atoms({"precise": True})
        node = db.get_atom(node_a.handle)
        link = db.get_atom(link_a.handle)
        assert count["atom_count"] == 3
        assert count["node_count"] == 2
        assert count["link_count"] == 1
        assert node.name == "B"
        assert link.targets == [node_a.handle]

    # Note no exception is raised if error
    def test_bulk_insert_exceptions(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = db._build_node(NodeT(name="A", type="A"))
        link_a = db._build_link(LinkT(targets=[node_a], type="A"))
        with pytest.raises(Exception):
            db.bulk_insert([node_a, link_a])
            # TODO: fix this
            pytest.skip(
                "ERROR should raise an exception. See https://github.com/singnet/das-atom-db/issues/210"
            )

    def test_retrieve_all_atoms(self, database, request):
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        link_a = add_link(
            db,
            "Test",
            [NodeT(name="A", type="A")],
            database,
        )
        node_b = db.get_atom(db.get_node_handle(node_type="A", node_name="A"))
        atoms = db.retrieve_all_atoms()
        assert isinstance(atoms, list)
        assert len(atoms) == 3
        all_atoms_handles = [a.handle for a in atoms]
        for atom in [node_a, link_a, node_b]:
            assert atom.handle in all_atoms_handles, f"{atom=}, {atoms=}"

    def test_commit(self, database, request):
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented on in_memory_db. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        node_a = db.add_node(NodeT(name="A", type="Test"))
        db.add_link(LinkT(type="Test", targets=[node_a]))
        count = db.count_atoms({"precise": True})
        assert count["atom_count"] == 0
        db.commit()
        count = db.count_atoms({"precise": True})
        assert count["atom_count"] == 2
        assert count["node_count"] == 1
        assert count["link_count"] == 1

    def test_commit_buffer(self, database, request):
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented on in_memory_db. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        node_a = add_node(db, "Aaa", "Test", database)
        link_a = add_link(
            db,
            "Test",
            [NodeT(name="A", type="A")],
            database,
        )

        node_a_dict = dict(
            name="B",  # different name
            _id=node_a._id,
            handle=node_a.handle,
            composite_type_hash=node_a.composite_type_hash,
            named_type=node_a.named_type,
        )
        link_a_dict = dict(
            targets=[node_a_dict["handle"]],  # different targets
            _id=link_a._id,
            handle=link_a.handle,
            composite_type_hash=link_a.composite_type_hash,
            named_type=link_a.named_type,
            composite_type=link_a.composite_type,
            named_type_hash=link_a.named_type_hash,
            is_toplevel=link_a.is_toplevel,
        )

        db.commit(buffer=[node_a_dict, link_a_dict])
        count = db.count_atoms({"precise": True})
        assert count["atom_count"] == 3
        assert count["node_count"] == 2
        assert count["link_count"] == 1

    def test_commit_buffer_exception(self, database, request):
        if database == "in_memory_db":
            # TODO: fix this
            pytest.skip(
                "ERROR Not implemented on in_memory_db. See https://github.com/singnet/das-atom-db/issues/210"
            )
        db: AtomDB = request.getfixturevalue(database)
        with pytest.raises(Exception):
            db.commit(buffer=[{"name": "A", "type": "A"}])

        with pytest.raises(Exception, match="Failed to commit Atom Types"):
            db.mongo_bulk_insertion_buffer = {"atom_types": ("a", "a")}
            db.commit()
