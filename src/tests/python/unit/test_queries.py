from hyperon_das_atomdb.database import LinkT, NodeT

from hyperon_das.constants import QueryOutputFormat
from hyperon_das.das import DistributedAtomSpace


def _name(link, index, typed=False):
    named_type = f"{link['targets'][index]['type']}:" if typed else ""
    return f"{named_type}{link['targets'][index]['name']}"


def _print_query_answer(query_answer, typed=False):
    if query_answer:
        for link in query_answer:
            if len(link["targets"]) == 2:
                print(f"{link['type']}: {_name(link, 0)} -> {_name(link, 1)}")
            elif len(link["targets"]) == 3:
                print(
                    f"{link['type']}: {_name(link, 0)}({_name(link, 1, typed)}) -> {_name(link, 2, typed)}"
                )
            else:
                assert False


def _check_query_answer(query_answer, expected_answer):
    assert len(query_answer) == len(expected_answer)
    for answer in query_answer:
        handles = [link.handle for link in answer[1]]
        for expected in expected_answer:
            if len(expected) != len(handles):
                continue
            if sorted(expected) == sorted(handles):
                break
        else:
            assert False


class TestQueries:
    def setup_animals_kb(self, das: DistributedAtomSpace):
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "human"}),
                        NodeT(**{"type": "Concept", "name": "monkey"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "human"}),
                        NodeT(**{"type": "Concept", "name": "chimp"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "chimp"}),
                        NodeT(**{"type": "Concept", "name": "monkey"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "snake"}),
                        NodeT(**{"type": "Concept", "name": "earthworm"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "rhino"}),
                        NodeT(**{"type": "Concept", "name": "triceratops"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "snake"}),
                        NodeT(**{"type": "Concept", "name": "vine"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "human"}),
                        NodeT(**{"type": "Concept", "name": "ent"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "monkey"}),
                        NodeT(**{"type": "Concept", "name": "human"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "chimp"}),
                        NodeT(**{"type": "Concept", "name": "human"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "monkey"}),
                        NodeT(**{"type": "Concept", "name": "chimp"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "earthworm"}),
                        NodeT(**{"type": "Concept", "name": "snake"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "triceratops"}),
                        NodeT(**{"type": "Concept", "name": "rhino"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "vine"}),
                        NodeT(**{"type": "Concept", "name": "snake"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Similarity",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "ent"}),
                        NodeT(**{"type": "Concept", "name": "human"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "human"}),
                        NodeT(**{"type": "Concept", "name": "mammal"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "monkey"}),
                        NodeT(**{"type": "Concept", "name": "mammal"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "chimp"}),
                        NodeT(**{"type": "Concept", "name": "mammal"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "mammal"}),
                        NodeT(**{"type": "Concept", "name": "animal"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "reptile"}),
                        NodeT(**{"type": "Concept", "name": "animal"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "snake"}),
                        NodeT(**{"type": "Concept", "name": "reptile"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "dinosaur"}),
                        NodeT(**{"type": "Concept", "name": "reptile"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "triceratops"}),
                        NodeT(**{"type": "Concept", "name": "dinosaur"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "earthworm"}),
                        NodeT(**{"type": "Concept", "name": "animal"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "rhino"}),
                        NodeT(**{"type": "Concept", "name": "mammal"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "vine"}),
                        NodeT(**{"type": "Concept", "name": "plant"}),
                    ],
                }
            )
        )
        das.add_link(
            LinkT(
                **{
                    "type": "Inheritance",
                    "targets": [
                        NodeT(**{"type": "Concept", "name": "ent"}),
                        NodeT(**{"type": "Concept", "name": "plant"}),
                    ],
                }
            )
        )

    def test_nested_pattern(self):
        das = DistributedAtomSpace()
        das.add_link(
            LinkT(
                type="Expression",
                targets=[
                    NodeT(type="Symbol", name="Test"),
                    LinkT(
                        type="Expression",
                        targets=[
                            NodeT(type="Symbol", name="Test"),
                            NodeT(type="Symbol", name="2"),
                        ],
                    ),
                ],
            )
        )
        query_params = {
            "toplevel_only": False,
            "return_type": QueryOutputFormat.ATOM_INFO,
            "no_iterator": True,
        }
        q1 = {
            "atom_type": "link",
            "type": "Expression",
            "targets": [
                {"atom_type": "variable", "name": "v1"},
                {
                    "atom_type": "link",
                    "type": "Expression",
                    "targets": [
                        {"atom_type": "variable", "name": "v2"},
                        {"atom_type": "node", "type": "Symbol", "name": "2"},
                    ],
                },
            ],
        }
        answer = das.query(q1, query_params)
        assert len(answer) == 1
        assert answer[0].subgraph.handle == "dbcf1c7b610a5adea335bf08f6509978"

    def test_conjunction(self):
        das = DistributedAtomSpace()
        self.setup_animals_kb(das)
        query_params = {"no_iterator": True}
        exp1 = {
            "atom_type": "link",
            "type": "Inheritance",
            "targets": [
                {"atom_type": "variable", "name": "v1"},
                {"atom_type": "variable", "name": "v2"},
            ],
        }
        exp2 = {
            "atom_type": "link",
            "type": "Inheritance",
            "targets": [
                {"atom_type": "variable", "name": "v2"},
                {"atom_type": "variable", "name": "v3"},
            ],
        }
        query_answer = das.query([exp1, exp2], query_params)
        answer = [tuple([item.assignment, item.subgraph]) for item in query_answer]
        _check_query_answer(
            answer,
            [
                [
                    "c93e1e758c53912638438e2a7d7f7b7f",
                    "1c3bf151ea200b2d9e088a1178d060cb",
                ],
                [
                    "f31dfe97db782e8cec26de18dddf8965",
                    "1c3bf151ea200b2d9e088a1178d060cb",
                ],
                [
                    "75756335011dcedb71a0d9a7bd2da9e8",
                    "1c3bf151ea200b2d9e088a1178d060cb",
                ],
                [
                    "116df61c01859c710d178ba14a483509",
                    "b0f428929706d1d991e4d712ad08f9ab",
                ],
                [
                    "959924e3aab197af80a84c1ab261fd65",
                    "b0f428929706d1d991e4d712ad08f9ab",
                ],
                [
                    "906fa505ae3bc6336d80a5f9aaa47b3b",
                    "959924e3aab197af80a84c1ab261fd65",
                ],
                [
                    "fbf03d17d6a40feff828a3f2c6e86f05",
                    "1c3bf151ea200b2d9e088a1178d060cb",
                ],
            ],
        )

    def test_returned_assignment(self):
        das = DistributedAtomSpace()
        self.setup_animals_kb(das)

        exp = {
            "atom_type": "link",
            "type": "Inheritance",
            "targets": [
                {"atom_type": "variable", "name": "v1"},
                {"atom_type": "variable", "name": "v2"},
            ],
        }
        for query_answer in das.query_engine._recursive_query(exp):
            link = query_answer.subgraph
            assignment = query_answer.assignment
            assert assignment.mapping["v1"] == link.targets_documents[0].handle
            assert assignment.mapping["v2"] == link.targets_documents[1].handle
