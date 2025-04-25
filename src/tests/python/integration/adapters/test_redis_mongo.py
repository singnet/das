import pytest

from hyperon_das_atomdb.adapters import RedisMongoDB
from hyperon_das_atomdb.adapters.redis_mongo_db import KeyPrefix
from hyperon_das_atomdb.database import WILDCARD, AtomDB, FieldIndexType, LinkT, NodeT
from hyperon_das_atomdb.utils.expression_hasher import ExpressionHasher
from tests.python.helpers import dict_to_link_params, dict_to_node_params

from .animals_kb import (
    animal,
    chimp,
    ent,
    human,
    inheritance,
    inheritance_docs,
    mammal,
    monkey,
    node_docs,
    rhino,
    similarity_docs,
)
from .helpers import (
    Database,
    PyMongoFindExplain,
    _db_down,
    _db_up,
    cleanup,
    mongo_port,
    redis_port,
)


def metta_to_links(input_str):
    def parse_tokens(tokens):
        result = []
        while tokens:
            token = tokens.pop(0)
            if token == "(":
                nested = parse_tokens(tokens)
                result.append(nested)
            elif token == ")":
                break
            else:
                result.append({"type": "Symbol", "name": token})
        return {"type": "Expression", "targets": result}

    input_str = input_str.replace("(", " ( ").replace(")", " ) ")
    tokens = input_str.split()
    return parse_tokens(tokens)["targets"][0]


class TestRedisMongo:
    @pytest.fixture(scope="session", autouse=True)
    def _cleanup(self, request):
        return cleanup(request)

    @pytest.fixture()
    def _db(self):
        _db_up(Database.REDIS, Database.MONGO)
        yield self._connect_db()
        _db_down()

    @pytest.fixture()
    def redis_mongo_up(self):
        _db_up(Database.REDIS, Database.MONGO)
        yield
        _db_down()

    def _add_atoms(self, db: RedisMongoDB):
        for node in node_docs.values():
            db.add_node(dict_to_node_params(node))
        for link in inheritance_docs.values():
            db.add_link(dict_to_link_params(link))
        for link in similarity_docs.values():
            db.add_link(dict_to_link_params(link))

    def _connect_db(self, extra_params: dict | None = None):
        params = {
            "mongo_port": mongo_port,
            "mongo_username": "dbadmin",
            "mongo_password": "dassecret",
            "redis_port": redis_port,
            "redis_cluster": False,
            "redis_ssl": False,
        }
        if extra_params:
            params.update(extra_params)
        db = RedisMongoDB(**params)
        return db

    def _check_basic_patterns(self, db, toplevel_only=False):
        answers = db.get_matched_links(
            "Inheritance",
            [WILDCARD, db.node_handle("Concept", "mammal")],
            toplevel_only=toplevel_only,
        )
        assert sorted([db.get_atom(answer).targets[0] for answer in answers]) == sorted(
            [human, monkey, chimp, rhino]
        )
        answers = db.get_matched_links(
            "Inheritance",
            [db.node_handle("Concept", "mammal"), WILDCARD],
            toplevel_only=toplevel_only,
        )
        assert sorted([db.get_atom(answer).targets[1] for answer in answers]) == sorted([animal])
        answers = db.get_matched_links(
            "Similarity",
            [WILDCARD, db.node_handle("Concept", "human")],
            toplevel_only=toplevel_only,
        )
        assert sorted([db.get_atom(answer).targets[0] for answer in answers]) == sorted(
            [monkey, chimp, ent]
        )
        answers = db.get_matched_links(
            "Similarity",
            [db.node_handle("Concept", "human"), WILDCARD],
            toplevel_only=toplevel_only,
        )
        assert sorted([db.get_atom(answer).targets[1] for answer in answers]) == sorted(
            [monkey, chimp, ent]
        )

    def test_redis_retrieve(self, _cleanup, _db: RedisMongoDB):
        db = _db
        self._add_atoms(db)
        assert db.count_atoms() == {"atom_count": 0}
        db.commit()
        assert db.count_atoms() == {"atom_count": 40}
        assert db.count_atoms({"precise": True}) == {
            "atom_count": 40,
            "node_count": 14,
            "link_count": 26,
        }
        assert db._retrieve_name(human) == "human"

        links = db._retrieve_incoming_set(mammal)
        assert len(links) == 5
        for link in links:
            outgoing = db._retrieve_outgoing_set(link)
            assert outgoing in [
                [human, mammal],
                [monkey, mammal],
                [chimp, mammal],
                [rhino, mammal],
                [mammal, animal],
            ]

        templates = db._retrieve_hash_targets_value(
            KeyPrefix.TEMPLATES, "41c082428b28d7e9ea96160f7fd614ad"
        )
        assert sorted(templates) == sorted(
            [
                "116df61c01859c710d178ba14a483509",
                "1c3bf151ea200b2d9e088a1178d060cb",
                "4120e428ab0fa162a04328e5217912ff",
                "75756335011dcedb71a0d9a7bd2da9e8",
                "906fa505ae3bc6336d80a5f9aaa47b3b",
                "959924e3aab197af80a84c1ab261fd65",
                "b0f428929706d1d991e4d712ad08f9ab",
                "c93e1e758c53912638438e2a7d7f7b7f",
                "e4685d56969398253b6f77efd21dc347",
                "ee1c03e6d1f104ccd811cfbba018451a",
                "f31dfe97db782e8cec26de18dddf8965",
                "fbf03d17d6a40feff828a3f2c6e86f05",
            ]
        )

        patterns = db._retrieve_hash_targets_value_from_sorted_set(
            KeyPrefix.PATTERNS, "112002ff70ea491aad735f978e9d95f5"
        )
        assert sorted(patterns) == sorted(
            [
                "75756335011dcedb71a0d9a7bd2da9e8",
                "fbf03d17d6a40feff828a3f2c6e86f05",
                "f31dfe97db782e8cec26de18dddf8965",
                "c93e1e758c53912638438e2a7d7f7b7f",
            ]
        )

    def test_patterns(self, _cleanup, _db: RedisMongoDB):
        db = _db
        self._add_atoms(db)
        db.commit()
        assert db.count_atoms() == {"atom_count": 40}
        self._check_basic_patterns(db)

    def test_commit(self, _cleanup, _db: RedisMongoDB):
        db = _db
        assert db.count_atoms() == {"atom_count": 0}
        self._add_atoms(db)
        assert db.count_atoms() == {"atom_count": 0}
        db.commit()
        assert db.count_atoms() == {"atom_count": 40}
        answers = db.get_matched_links(
            "Inheritance", [WILDCARD, db.node_handle("Concept", "mammal")]
        )
        assert sorted([db.get_atom(answer).targets[0] for answer in answers]) == sorted(
            [human, monkey, chimp, rhino]
        )
        assert db.get_atom(human).name == node_docs[human]["name"]
        link_pre = db.get_atom(inheritance[human][mammal])
        assert link_pre.custom_attributes == dict()
        assert link_pre.named_type == "Inheritance"
        assert link_pre.targets == [human, mammal]
        link_new = inheritance_docs[inheritance[human][mammal]].copy()
        custom_attributes = {"strength": 1.0}
        link_new["custom_attributes"] = custom_attributes
        db.add_link(dict_to_link_params(link_new))
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
        db.commit()
        assert db.count_atoms({"precise": True}) == {
            "atom_count": 42,
            "node_count": 15,
            "link_count": 27,
        }
        link_pos = db.get_atom(inheritance[human][mammal])
        assert link_pos.named_type == "Inheritance"
        assert link_pos.targets == [human, mammal]
        assert isinstance(link_pos.custom_attributes, dict)
        assert "strength" in link_pos.custom_attributes
        assert isinstance(link_pos.custom_attributes["strength"], float)
        assert link_pos.custom_attributes["strength"] == 1.0
        dog = db.node_handle("Concept", "dog")
        assert db.get_node_name(dog) == "dog"
        new_link_handle = db.get_link_handle("Inheritance", [dog, mammal])
        new_link = db.get_atom(new_link_handle)
        assert db.get_link_targets(new_link_handle) == new_link.targets
        answers = db.get_matched_links(
            "Inheritance", [WILDCARD, db.node_handle("Concept", "mammal")]
        )
        assert sorted([db.get_atom(answer).targets[0] for answer in answers]) == sorted(
            [human, monkey, chimp, rhino, dog]
        )

    def test_reindex(self, _cleanup, _db: RedisMongoDB):
        db = _db
        self._add_atoms(db)
        db.commit()
        db.reindex()
        assert db.count_atoms() == {"atom_count": 40}
        self._check_basic_patterns(db)
        _db_down()

    def test_delete_atom(self, _cleanup, _db: RedisMongoDB):
        def _add_all_links():
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
            db.commit()

        def _add_nested_links():
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
            db.commit()

        def _check_asserts():
            assert db.count_atoms({"precise": True}) == {
                "atom_count": 5,
                "node_count": 3,
                "link_count": 2,
            }
            assert db._retrieve_name(cat_handle) == "cat"
            assert db._retrieve_name(dog_handle) == "dog"
            assert db._retrieve_name(mammal_handle) == "mammal"
            assert db._retrieve_incoming_set(cat_handle) == {inheritance_cat_mammal_handle}
            assert db._retrieve_incoming_set(dog_handle) == {inheritance_dog_mammal_handle}
            assert sorted(db._retrieve_incoming_set(mammal_handle)) == sorted(
                [inheritance_cat_mammal_handle, inheritance_dog_mammal_handle]
            )
            assert db._retrieve_incoming_set(inheritance_cat_mammal_handle) == set()
            assert db._retrieve_incoming_set(inheritance_dog_mammal_handle) == set()
            assert sorted(db._retrieve_outgoing_set(inheritance_cat_mammal_handle)) == sorted(
                [cat_handle, mammal_handle]
            )
            assert sorted(db._retrieve_outgoing_set(inheritance_dog_mammal_handle)) == sorted(
                [dog_handle, mammal_handle]
            )
            assert sorted(
                db._retrieve_hash_targets_value(
                    KeyPrefix.TEMPLATES, "e40489cd1e7102e35469c937e05c8bba"
                )
            ) == sorted([inheritance_dog_mammal_handle, inheritance_cat_mammal_handle])
            assert sorted(
                db._retrieve_hash_targets_value(
                    KeyPrefix.TEMPLATES, "41c082428b28d7e9ea96160f7fd614ad"
                )
            ) == sorted([inheritance_dog_mammal_handle, inheritance_cat_mammal_handle])

            links = [
                db.get_atom(inheritance_cat_mammal_handle),
                db.get_atom(inheritance_dog_mammal_handle),
            ]
            keys = set()
            for link in links:
                for template in db.pattern_index_templates:
                    key = db._apply_index_template(
                        template,
                        link.named_type_hash,
                        link.targets,
                        len(link.targets),
                    )
                    keys.add(key)
            assert set([p for p in db.redis.keys("patterns:*")]) == keys

            assert sorted(
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "112002ff70ea491aad735f978e9d95f5"
                )
            ) == sorted([inheritance_dog_mammal_handle, inheritance_cat_mammal_handle])
            assert sorted(
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "6e644e70a9fe3145c88b5b6261af5754"
                )
            ) == sorted([inheritance_dog_mammal_handle, inheritance_cat_mammal_handle])
            assert sorted(
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "5dd515aa7a451276feac4f8b9d84ae91"
                )
            ) == sorted([inheritance_dog_mammal_handle, inheritance_cat_mammal_handle])
            assert sorted(
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "7ead6cfa03894c62761162b7603aa885"
                )
            ) == sorted([inheritance_dog_mammal_handle, inheritance_cat_mammal_handle])
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "e55007a8477a4e6bf4fec76e4ffd7e10"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "23dc149b3218d166a14730db55249126"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "399751d7319f9061d97cd1d75728b66b"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "d0eaae6eaf750e821b26642cef32becf"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "f29daafee640d91aa7091e44551fc74a"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "a11d7cbf62bc544f75702b5fb6a514ff"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "3ba42d45a50c89600d92fb3f1a46c1b5"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "9fb71ffef74a1a98eb0bfce7aa3d54e3"
            ) == {inheritance_cat_mammal_handle}

        def _check_asserts_2():
            assert db.count_atoms() == {"atom_count": 3}
            assert db._retrieve_name(cat_handle) == "cat"
            assert db._retrieve_name(dog_handle) == "dog"
            assert db._retrieve_name(mammal_handle) == "mammal"
            assert db._retrieve_incoming_set(cat_handle) == set()
            assert db._retrieve_incoming_set(dog_handle) == set()
            assert db._retrieve_incoming_set(mammal_handle) == set()
            assert db._retrieve_outgoing_set(inheritance_cat_mammal_handle) == []
            assert db._retrieve_outgoing_set(inheritance_dog_mammal_handle) == []
            assert (
                db._retrieve_hash_targets_value(
                    KeyPrefix.TEMPLATES, "e40489cd1e7102e35469c937e05c8bba"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value(
                    KeyPrefix.TEMPLATES, "41c082428b28d7e9ea96160f7fd614ad"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "112002ff70ea491aad735f978e9d95f5"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "6e644e70a9fe3145c88b5b6261af5754"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "5dd515aa7a451276feac4f8b9d84ae91"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "7ead6cfa03894c62761162b7603aa885"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "e55007a8477a4e6bf4fec76e4ffd7e10"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "23dc149b3218d166a14730db55249126"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "399751d7319f9061d97cd1d75728b66b"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "d0eaae6eaf750e821b26642cef32becf"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "f29daafee640d91aa7091e44551fc74a"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "a11d7cbf62bc544f75702b5fb6a514ff"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "3ba42d45a50c89600d92fb3f1a46c1b5"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "9fb71ffef74a1a98eb0bfce7aa3d54e3"
                )
                == set()
            )

        def _check_asserts_3():
            assert db.count_atoms() == {"atom_count": 2}
            assert db._retrieve_name(cat_handle) == "cat"
            assert db._retrieve_name(dog_handle) == "dog"
            assert db._retrieve_name(mammal_handle) is None
            assert db._retrieve_incoming_set(cat_handle) == set()
            assert db._retrieve_incoming_set(dog_handle) == set()
            assert db._retrieve_incoming_set(mammal_handle) == set()
            assert db._retrieve_outgoing_set(inheritance_cat_mammal_handle) == []
            assert db._retrieve_outgoing_set(inheritance_dog_mammal_handle) == []
            assert (
                db._retrieve_hash_targets_value(
                    KeyPrefix.TEMPLATES, "e40489cd1e7102e35469c937e05c8bba"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value(
                    KeyPrefix.TEMPLATES, "41c082428b28d7e9ea96160f7fd614ad"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "112002ff70ea491aad735f978e9d95f5"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "6e644e70a9fe3145c88b5b6261af5754"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "5dd515aa7a451276feac4f8b9d84ae91"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "7ead6cfa03894c62761162b7603aa885"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "e55007a8477a4e6bf4fec76e4ffd7e10"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "23dc149b3218d166a14730db55249126"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "399751d7319f9061d97cd1d75728b66b"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "d0eaae6eaf750e821b26642cef32becf"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "f29daafee640d91aa7091e44551fc74a"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "a11d7cbf62bc544f75702b5fb6a514ff"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "3ba42d45a50c89600d92fb3f1a46c1b5"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "9fb71ffef74a1a98eb0bfce7aa3d54e3"
                )
                == set()
            )

        def _check_asserts_4():
            assert db.count_atoms({"precise": True}) == {
                "atom_count": 3,
                "node_count": 2,
                "link_count": 1,
            }
            assert db._retrieve_name(cat_handle) is None
            assert db._retrieve_name(dog_handle) == "dog"
            assert db._retrieve_name(mammal_handle) == "mammal"
            assert db._retrieve_incoming_set(cat_handle) == set()
            assert db._retrieve_incoming_set(dog_handle) == {inheritance_dog_mammal_handle}
            assert db._retrieve_incoming_set(mammal_handle) == {inheritance_dog_mammal_handle}
            assert db._retrieve_outgoing_set(inheritance_cat_mammal_handle) == []
            assert sorted(db._retrieve_outgoing_set(inheritance_dog_mammal_handle)) == sorted(
                [dog_handle, mammal_handle]
            )
            assert db._retrieve_hash_targets_value(
                KeyPrefix.TEMPLATES, "e40489cd1e7102e35469c937e05c8bba"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value(
                KeyPrefix.TEMPLATES, "41c082428b28d7e9ea96160f7fd614ad"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "112002ff70ea491aad735f978e9d95f5"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "6e644e70a9fe3145c88b5b6261af5754"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "5dd515aa7a451276feac4f8b9d84ae91"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "7ead6cfa03894c62761162b7603aa885"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "e55007a8477a4e6bf4fec76e4ffd7e10"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "23dc149b3218d166a14730db55249126"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "399751d7319f9061d97cd1d75728b66b"
            ) == {inheritance_dog_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "d0eaae6eaf750e821b26642cef32becf"
            ) == {inheritance_dog_mammal_handle}
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "f29daafee640d91aa7091e44551fc74a"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "a11d7cbf62bc544f75702b5fb6a514ff"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "3ba42d45a50c89600d92fb3f1a46c1b5"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "9fb71ffef74a1a98eb0bfce7aa3d54e3"
                )
                == set()
            )

        def _check_asserts_5():
            assert db.count_atoms({"precise": True}) == {
                "atom_count": 3,
                "node_count": 2,
                "link_count": 1,
            }
            assert db._retrieve_name(cat_handle) == "cat"
            assert db._retrieve_name(dog_handle) is None
            assert db._retrieve_name(mammal_handle) == "mammal"
            assert db._retrieve_incoming_set(cat_handle) == {inheritance_cat_mammal_handle}
            assert db._retrieve_incoming_set(dog_handle) == set()
            assert db._retrieve_incoming_set(mammal_handle) == {inheritance_cat_mammal_handle}
            assert sorted(db._retrieve_outgoing_set(inheritance_cat_mammal_handle)) == sorted(
                [cat_handle, mammal_handle]
            )
            assert db._retrieve_outgoing_set(inheritance_dog_mammal_handle) == []
            assert db._retrieve_hash_targets_value(
                KeyPrefix.TEMPLATES, "e40489cd1e7102e35469c937e05c8bba"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value(
                KeyPrefix.TEMPLATES, "41c082428b28d7e9ea96160f7fd614ad"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "112002ff70ea491aad735f978e9d95f5"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "6e644e70a9fe3145c88b5b6261af5754"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "5dd515aa7a451276feac4f8b9d84ae91"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "7ead6cfa03894c62761162b7603aa885"
            ) == {inheritance_cat_mammal_handle}
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "e55007a8477a4e6bf4fec76e4ffd7e10"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "23dc149b3218d166a14730db55249126"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "399751d7319f9061d97cd1d75728b66b"
                )
                == set()
            )
            assert (
                db._retrieve_hash_targets_value_from_sorted_set(
                    KeyPrefix.PATTERNS, "d0eaae6eaf750e821b26642cef32becf"
                )
                == set()
            )
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "f29daafee640d91aa7091e44551fc74a"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "a11d7cbf62bc544f75702b5fb6a514ff"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "3ba42d45a50c89600d92fb3f1a46c1b5"
            ) == {inheritance_cat_mammal_handle}
            assert db._retrieve_hash_targets_value_from_sorted_set(
                KeyPrefix.PATTERNS, "9fb71ffef74a1a98eb0bfce7aa3d54e3"
            ) == {inheritance_cat_mammal_handle}

        db = _db

        cat_handle = AtomDB.node_handle("Concept", "cat")
        dog_handle = AtomDB.node_handle("Concept", "dog")
        mammal_handle = AtomDB.node_handle("Concept", "mammal")
        inheritance_cat_mammal_handle = AtomDB.link_handle(
            "Inheritance", [cat_handle, mammal_handle]
        )
        inheritance_dog_mammal_handle = AtomDB.link_handle(
            "Inheritance", [dog_handle, mammal_handle]
        )

        assert db.count_atoms() == {"atom_count": 0}

        _add_all_links()
        _check_asserts()

        db.delete_atom(inheritance_cat_mammal_handle)
        db.delete_atom(inheritance_dog_mammal_handle)
        _check_asserts_2()

        _add_all_links()
        db.delete_atom(mammal_handle)
        _check_asserts_3()

        _add_all_links()
        db.delete_atom(cat_handle)
        _check_asserts_4()

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
        db.commit()

        db.delete_atom(dog_handle)
        _check_asserts_5()

        db.clear_database()

        _add_nested_links()
        db.delete_atom(inheritance_cat_mammal_handle)
        _check_asserts_2()

    def test_get_matched_with_pagination(self, _cleanup, _db: RedisMongoDB):
        db = _db
        self._add_atoms(db)
        db.commit()

        response = db.get_matched_links("Similarity", [human, monkey])
        assert response == {AtomDB.link_handle("Similarity", [human, monkey])}

        response = db.get_matched_links("Fake", [human, monkey])
        assert response == set()

        response = db.get_matched_links("Similarity", [human, "*"])
        assert sorted(response) == sorted(
            [
                "16f7e407087bfa0b35b13d13a1aadcae",
                "b5459e299a5c5e8662c427f7e01b3bf1",
                "bad7472f41a0e7d601ca294eb4607c3a",
            ]
        )

        template = ["Inheritance", "Concept", "Concept"]

        response = db.get_matched_type_template(template)
        assert sorted(response) == sorted(
            [
                "116df61c01859c710d178ba14a483509",
                "1c3bf151ea200b2d9e088a1178d060cb",
                "4120e428ab0fa162a04328e5217912ff",
                "75756335011dcedb71a0d9a7bd2da9e8",
                "906fa505ae3bc6336d80a5f9aaa47b3b",
                "959924e3aab197af80a84c1ab261fd65",
                "b0f428929706d1d991e4d712ad08f9ab",
                "c93e1e758c53912638438e2a7d7f7b7f",
                "e4685d56969398253b6f77efd21dc347",
                "ee1c03e6d1f104ccd811cfbba018451a",
                "f31dfe97db782e8cec26de18dddf8965",
                "fbf03d17d6a40feff828a3f2c6e86f05",
            ]
        )

        response = db.get_matched_type("Inheritance")
        assert sorted(response) == sorted(
            [
                "116df61c01859c710d178ba14a483509",
                "1c3bf151ea200b2d9e088a1178d060cb",
                "4120e428ab0fa162a04328e5217912ff",
                "75756335011dcedb71a0d9a7bd2da9e8",
                "906fa505ae3bc6336d80a5f9aaa47b3b",
                "959924e3aab197af80a84c1ab261fd65",
                "b0f428929706d1d991e4d712ad08f9ab",
                "c93e1e758c53912638438e2a7d7f7b7f",
                "e4685d56969398253b6f77efd21dc347",
                "ee1c03e6d1f104ccd811cfbba018451a",
                "f31dfe97db782e8cec26de18dddf8965",
                "fbf03d17d6a40feff828a3f2c6e86f05",
            ]
        )

    def test_create_field_index(self, _cleanup, _db: RedisMongoDB):
        db = _db
        self._add_atoms(db)
        db.commit()
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Similarity",
                    "targets": [
                        {"type": "Concept", "name": "human"},
                        {"type": "Concept", "name": "monkey"},
                    ],
                    "custom_attributes": {"tag": "DAS"},
                }
            )
        )
        db.commit()

        collection = db.mongo_atoms_collection

        response = collection.find(
            {"named_type": "Similarity", "custom_attributes.tag": "DAS"}
        ).explain()

        with pytest.raises(KeyError):
            response["queryPlanner"]["winningPlan"]["inputStage"]["indexName"]

        # Create the index
        my_index = db.create_field_index(
            atom_type="link", fields=["custom_attributes.tag"], named_type="Similarity"
        )

        collection_index_names = [idx.get("name") for idx in collection.list_indexes()]
        #
        assert my_index in collection_index_names

        # # Using the index
        response = collection.find(
            {"named_type": "Similarity", "custom_attributes.tag": "DAS"}
        ).explain()

        assert my_index == response["queryPlanner"]["winningPlan"]["inputStage"]["indexName"]

        with PyMongoFindExplain(db.mongo_atoms_collection) as explain:
            _, doc = db.get_atoms_by_index(
                my_index, [{"field": "custom_attributes.tag", "value": "DAS"}]
            )
            assert doc[0].handle == ExpressionHasher.expression_hash(
                ExpressionHasher.named_type_hash("Similarity"), [human, monkey]
            )
            assert doc[0].targets == [human, monkey]
            assert explain[0]["executionStats"]["executionSuccess"]
            assert explain[0]["executionStats"]["executionStages"]["docsExamined"] == 1
            assert explain[0]["executionStats"]["executionStages"]["stage"] == "FETCH"
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["stage"] == "IXSCAN"
            )
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["keysExamined"] == 1
            )
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["indexName"]
                == my_index
            )

    def test_create_text_index(self, _cleanup, _db: RedisMongoDB):
        db: RedisMongoDB = _db
        self._add_atoms(db)
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Similarity",
                    "targets": [
                        {"type": "Concept", "name": "human"},
                        {"type": "Concept", "name": "monkey"},
                    ],
                    "custom_attributes": {"tag": "DAS"},
                }
            )
        )
        db.commit()

        collection = db.mongo_atoms_collection

        # Create the index
        my_index = db.create_field_index(
            atom_type="link",
            fields=["custom_attributes.tag"],
            named_type="Similarity",
            index_type=FieldIndexType.TOKEN_INVERTED_LIST,
        )

        collection_index_names = [idx.get("name") for idx in collection.list_indexes()]
        print(my_index)
        assert my_index in collection_index_names

    def test_create_compound_index(self, _cleanup, _db: RedisMongoDB):
        db: RedisMongoDB = _db
        self._add_atoms(db)
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Similarity",
                    "targets": [
                        {"type": "Concept", "name": "human"},
                        {"type": "Concept", "name": "monkey"},
                    ],
                    "custom_attributes": {"tag": "DAS"},
                }
            )
        )
        db.commit()
        collection = db.mongo_atoms_collection
        # Create the index
        my_index = db.create_field_index(
            atom_type="link",
            fields=["custom_attributes.type", "custom_attributes.tag"],
            named_type="Similarity",
            index_type=FieldIndexType.BINARY_TREE,
        )
        collection_index_names = [idx.get("name") for idx in collection.list_indexes()]
        assert my_index in collection_index_names

    def test_get_atoms_by_field_no_index(self, _cleanup, _db: RedisMongoDB):
        db: RedisMongoDB = _db
        self._add_atoms(db)
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Similarity",
                    "targets": [
                        {"type": "Concept", "name": "human"},
                        {"type": "Concept", "name": "monkey"},
                    ],
                    "custom_attributes": {"tag": "DAS"},
                }
            )
        )
        db.commit()

        with PyMongoFindExplain(db.mongo_atoms_collection) as explain:
            result = db.get_atoms_by_field([{"field": "custom_attributes.tag", "value": "DAS"}])
            assert len(result) == 1
            assert explain[0]["executionStats"]["executionSuccess"]
            assert explain[0]["queryPlanner"]["winningPlan"]["stage"] == "COLLSCAN"
            assert explain[0]["executionStats"]["totalKeysExamined"] == 0

    def test_get_atoms_by_field_with_index(self, _cleanup, _db: RedisMongoDB):
        # pytest.skip(
        #     "Requires new implementation since the new custom attributes were introduced. See https://github.com/singnet/das-atom-db/issues/255"
        # )
        db: RedisMongoDB = _db
        self._add_atoms(db)
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Similarity",
                    "targets": [
                        {"type": "Concept", "name": "human"},
                        {"type": "Concept", "name": "monkey"},
                    ],
                    "custom_attributes": {"tag": "DAS"},
                }
            )
        )
        db.commit()
        my_index = db.create_field_index(atom_type="link", fields=["custom_attributes.tag"])

        with PyMongoFindExplain(db.mongo_atoms_collection) as explain:
            result = db.get_atoms_by_field([{"field": "custom_attributes.tag", "value": "DAS"}])
            assert len(result) == 1
            assert explain[0]["executionStats"]["executionSuccess"]
            assert explain[0]["executionStats"]["nReturned"] == 1
            assert explain[0]["executionStats"]["executionStages"]["stage"] == "FETCH"
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["stage"] == "IXSCAN"
            )
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["keysExamined"] == 1
            )
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["indexName"]
                == my_index
            )

    def test_get_atoms_by_index(self, _cleanup, _db: RedisMongoDB):
        db: RedisMongoDB = _db
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Similarity",
                    "targets": [
                        {"type": "Concept", "name": "human"},
                        {"type": "Concept", "name": "monkey"},
                    ],
                    "custom_attributes": {"tag": "DAS"},
                }
            )
        )
        db.add_link(
            dict_to_link_params(
                {
                    "type": "Similarity",
                    "targets": [
                        {"type": "Concept", "name": "mammal"},
                        {"type": "Concept", "name": "monkey"},
                    ],
                    "custom_attributes": {"tag": "DAS2"},
                }
            )
        )
        db.commit()

        my_index = db.create_field_index(
            atom_type="link", fields=["custom_attributes.tag"], named_type="Similarity"
        )

        with PyMongoFindExplain(db.mongo_atoms_collection) as explain:
            _, doc = db.get_atoms_by_index(
                my_index, [{"field": "custom_attributes.tag", "value": "DAS2"}]
            )
            assert doc[0].handle == ExpressionHasher.expression_hash(
                ExpressionHasher.named_type_hash("Similarity"), [mammal, monkey]
            )
            assert doc[0].targets == [mammal, monkey]
            assert explain[0]["executionStats"]["executionSuccess"]
            assert explain[0]["executionStats"]["nReturned"] == 1
            assert explain[0]["executionStats"]["executionStages"]["stage"] == "FETCH"
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["stage"] == "IXSCAN"
            )
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["keysExamined"] == 1
            )
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["indexName"]
                == my_index
            )

    def test_get_atoms_by_text_field_regex(self, _cleanup, _db: RedisMongoDB):
        db: RedisMongoDB = _db
        self._add_atoms(db)
        db.commit()

        with PyMongoFindExplain(db.mongo_atoms_collection) as explain:
            result = db.get_atoms_by_text_field("mammal", "name")
            assert len(result) == 1
            assert result[0] == db.get_node_handle("Concept", "mammal")
            assert explain[0]["executionStats"]["executionSuccess"]
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["stage"] == "IXSCAN"
            )
            assert explain[0]["executionStats"]["totalKeysExamined"] == 14

    def test_get_atoms_by_text_field_with_index(self, _cleanup, _db: RedisMongoDB):
        db: RedisMongoDB = _db
        self._add_atoms(db)
        db.commit()
        # Create index

        db.create_field_index(
            atom_type="node",
            fields=["custom_attributes.name"],
            index_type=FieldIndexType.TOKEN_INVERTED_LIST,
        )

        with PyMongoFindExplain(db.mongo_atoms_collection) as explain:
            result = db.get_atoms_by_text_field("custom_attributes.mammal")
            assert len(result) == 1
            assert result[0] == db.get_node_handle("Concept", "mammal")
            assert explain[0]["executionStats"]["executionSuccess"]
            assert explain[0]["executionStats"]["executionStages"]["stage"] == "TEXT_MATCH"
            assert explain[0]["executionStats"]["totalKeysExamined"] == 1

    def test_get_node_starting_name(self, _cleanup, _db: RedisMongoDB):
        db = _db
        self._add_atoms(db)
        db.commit()
        with PyMongoFindExplain(db.mongo_atoms_collection) as explain:
            result = db.get_node_by_name_starting_with("Concept", "mamm")
            assert len(result) == 1
            assert result[0] == db.get_node_handle("Concept", "mammal")
            assert explain[0]["executionStats"]["executionSuccess"]
            assert (
                explain[0]["executionStats"]["executionStages"]["inputStage"]["stage"] == "IXSCAN"
            )
            assert explain[0]["executionStats"]["totalKeysExamined"] == 2
            assert explain[0]["executionStats"]["totalDocsExamined"] == 1

    def test_bulk_insert(self, _cleanup, _db: RedisMongoDB):
        db = _db
        assert db.count_atoms() == {"atom_count": 0}

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
        ]
        handle = db.link_handle("Similarity", ["node1", "node2"])
        documents.append(
            LinkT(
                _id=handle,
                handle=handle,
                composite_type_hash="CompositeTypeHash",
                is_toplevel=True,
                composite_type=["SimilarityHash", "ConceptHash", "ConceptHash"],
                named_type="Similarity",
                named_type_hash="SimilarityHash",
                targets=["node1", "node2"],
            ),
        )

        db.bulk_insert(documents)

        assert db.count_atoms() == {"atom_count": 3}
        assert db.get_matched_links("Similarity", ["node1", "node2"]) == {
            db.link_handle("Similarity", ["node1", "node2"])
        }
        similarity = db.get_all_links("Similarity")
        assert similarity == {db.link_handle("Similarity", ["node1", "node2"])}
        assert db.get_all_nodes_handles("Concept") == ["node1", "node2"]

    def test_retrieve_all_atoms(self, _cleanup, _db: RedisMongoDB):
        db = _db
        self._add_atoms(db)
        db.commit()
        response = db.retrieve_all_atoms()
        inheritance = db.get_all_links("Inheritance")
        similarity = db.get_all_links("Similarity")
        links = inheritance.union(similarity)
        nodes = db.get_all_nodes_handles("Concept")
        assert len(response) == len(links) + len(nodes)

    def test_add_fields_to_atoms(self, _cleanup, _db: RedisMongoDB):
        db = _db
        self._add_atoms(db)
        db.commit()
        human = db.node_handle("Concept", "human")
        monkey = db.node_handle("Concept", "monkey")
        link_handle = db.link_handle("Similarity", [human, monkey])

        node_human = db.get_atom(human)

        assert node_human.handle == human
        assert node_human.name == "human"
        assert node_human.named_type == "Concept"

        node_human_params = node_human
        node_human_params.custom_attributes = {"score": 0.5}

        db.add_node(node_human_params)
        db.commit()

        assert db.get_atom(human).custom_attributes["score"] == 0.5

        link_similarity = db.get_atom(link_handle, deep_representation=True)

        assert link_similarity.handle == link_handle
        assert link_similarity.named_type == "Similarity"
        assert [target.to_dict() for target in link_similarity.targets_documents] == [
            db.get_atom(human).to_dict(),
            db.get_atom(monkey).to_dict(),
        ]

        link_params = link_similarity
        link_params.custom_attributes = {"score": 0.5}

        db.add_link(link_params)
        db.commit()

        assert db.get_atom(link_handle).custom_attributes["score"] == 0.5

    @pytest.mark.parametrize(
        "node",
        [({"type": "A", "name": "type_a", "custom_attributes": {"status": "ready"}})],
    )
    def test_get_atoms_by_index_custom_att(self, node, _cleanup, _db: RedisMongoDB):
        node = _db.add_node(NodeT(**node))
        _db.commit()
        result = _db.create_field_index("node", fields=["custom_attributes.status"])
        cursor, actual = _db.get_atoms_by_index(
            result, [{"field": "custom_attributes.status", "value": "ready"}]
        )
        assert cursor == 0
        assert isinstance(actual, list)
        assert len(actual) == 1
        assert all([a.handle == node.handle for a in actual])

    def test_commit_with_buffer(self, _cleanup, _db: RedisMongoDB):
        db = _db
        assert db.count_atoms() == {"atom_count": 0}
        buffer = [
            {
                "_id": "26d35e45817f4270f2b7cff971b04138",
                "composite_type_hash": "d99a604c79ce3c2e76a2f43488d5d4c3",
                "name": "dog",
                "named_type": "Concept",
            },
            {
                "_id": "b7db6a9ed2191eb77ee54479570db9a4",
                "composite_type_hash": "d99a604c79ce3c2e76a2f43488d5d4c3",
                "name": "cat",
                "named_type": "Concept",
            },
            {
                "_id": "3dab102938606f4549d68405ec9f4f61",
                "composite_type_hash": "ed73ea081d170e1d89fc950820ce1cee",
                "is_toplevel": True,
                "composite_type": [
                    "a9dea78180588431ec64d6bc4872fdbc",
                    "d99a604c79ce3c2e76a2f43488d5d4c3",
                    "d99a604c79ce3c2e76a2f43488d5d4c3",
                ],
                "named_type": "Similarity",
                "named_type_hash": "a9dea78180588431ec64d6bc4872fdbc",
                "key_0": "26d35e45817f4270f2b7cff971b04138",
                "key_1": "b7db6a9ed2191eb77ee54479570db9a4",
            },
        ]
        db.commit(buffer=buffer)
        assert db.count_atoms({"precise": True}) == {
            "atom_count": 3,
            "node_count": 2,
            "link_count": 1,
        }
        assert db.get_atom("26d35e45817f4270f2b7cff971b04138").name == "dog"
        assert db.get_atom("b7db6a9ed2191eb77ee54479570db9a4").name == "cat"
        assert db.get_atom("3dab102938606f4549d68405ec9f4f61").targets == [
            "26d35e45817f4270f2b7cff971b04138",
            "b7db6a9ed2191eb77ee54479570db9a4",
        ]

    @pytest.mark.parametrize(
        "template,metta_link,queries,expected",
        [
            (
                [
                    {
                        "field": "targets[0]",
                        "value": ExpressionHasher.terminal_hash("Symbol", "Similarity"),
                        "positions": [1, 2],
                        "arity": 3,
                    }
                ],
                '(Similarity "Human" "Monkey")',
                [
                    ("Similarity", "*", '"Monkey"'),
                    ("Similarity", '"Human"', "*"),
                    ("Similarity", '"Human"', "*"),
                    ("Similarity", "*", "*"),
                ],
                8,
            ),
            (
                [
                    {
                        "field": "targets[0]",
                        "value": ExpressionHasher.terminal_hash("Symbol", "transcribed_to"),
                        "positions": [1, 2],
                        "arity": 3,
                    }
                ],
                '(transcribed_to (gene "ENSG00000290825") (transcript "ENST00000456328"))',
                [
                    ("transcribed_to", "*", "*"),
                    (
                        "transcribed_to",
                        ExpressionHasher.composite_hash(
                            [
                                ExpressionHasher.named_type_hash("Expression"),
                                ExpressionHasher.terminal_hash("Symbol", "gene"),
                                ExpressionHasher.terminal_hash("Symbol", '"ENSG00000290825"'),
                            ]
                        ),
                        "*",
                    ),
                    (
                        "transcribed_to",
                        "*",
                        ExpressionHasher.composite_hash(
                            [
                                ExpressionHasher.named_type_hash("Expression"),
                                ExpressionHasher.terminal_hash("Symbol", "transcript"),
                                ExpressionHasher.terminal_hash("Symbol", '"ENST00000456328"'),
                            ]
                        ),
                    ),
                ],
                8,
            ),
            (
                [
                    {
                        "field": "targets[0]",
                        "value": ExpressionHasher.terminal_hash("Symbol", "synonyms"),
                        "positions": [1, 2],
                        "arity": 3,
                    }
                ],
                "(synonyms (gene ENSG00000278267) (microRNA_6859-1 hsa-mir-6859-1 HGNC:50039 microRNA_mir-6859-1 MIR6859-1))",
                [
                    ("synonyms", "*", "*"),
                ],
                8,
            ),
            (
                [
                    {
                        "field": "targets[0]",
                        "value": ExpressionHasher.terminal_hash("Symbol", "tf_name"),
                        "positions": [1, 2],
                        "arity": 3,
                    }
                ],
                "(tf_name (motif ENSG00000156273) BACH1)",
                [
                    ("tf_name", "*", "*"),
                ],
                8,
            ),
        ],
    )
    def test_index_pattern_generation(
        self, template, metta_link, queries, expected, _cleanup, redis_mongo_up
    ):
        db: RedisMongoDB = self._connect_db({"pattern_index_templates": template})
        db.add_link(dict_to_link_params(metta_to_links(metta_link)))
        db.commit()
        for q in queries:
            tt = [
                n if n == "*" or len(n) == 32 else ExpressionHasher.terminal_hash("Symbol", n)
                for n in q
            ]
            links: set[str] = db.get_matched_links("*", tt)
            assert len(links) == 1
        assert len(db.pattern_index_templates) == expected
