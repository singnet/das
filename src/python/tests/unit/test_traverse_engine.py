import pytest
from hyperon_das_atomdb import AtomDB, AtomDoesNotExist

from hyperon_das.das import DistributedAtomSpace
from tests.helpers import animal_base_handles, load_animals_base


def simplify_links(links: list, das: DistributedAtomSpace) -> set:
    answers = set()
    for link in links:
        targets_name = []
        for target in link['targets']:
            targets_name.append(das.get_atom(target)['name'])
        answers.add(f"{link['named_type']} : {targets_name}")
    return answers


def get_names(handles: str, das: DistributedAtomSpace) -> set:
    answer = set()
    for handle in handles:
        answer.add(das.get_atom(handle)['name'])
    return answer


@pytest.mark.skip(
    reason="Waiting for integration with cache sub-module https://github.com/singnet/das/issues/73"
)
class TestTraverseEngine:
    @pytest.fixture
    def das(self):
        das = DistributedAtomSpace()
        load_animals_base(das)
        return das

    def test_get(self, das):
        cursor = das.get_traversal_cursor(animal_base_handles.human)
        current_cursor = cursor.get()

        assert current_cursor['handle'] == animal_base_handles.human
        assert current_cursor['name'] == 'human'
        assert current_cursor['named_type'] == 'Concept'

    def test_get_links(self, das):
        def _build_atom_answer(handle: str) -> dict:
            cursor = das.get_traversal_cursor(handle)
            links = cursor.get_links()
            return simplify_links(links, das)

        def _human_links():
            answers = _build_atom_answer(animal_base_handles.human)
            assert len(answers) == 7
            assert answers == {
                "Similarity : ['human', 'chimp']",
                "Similarity : ['human', 'monkey']",
                "Similarity : ['human', 'ent']",
                "Similarity : ['ent', 'human']",
                "Similarity : ['monkey', 'human']",
                "Similarity : ['chimp', 'human']",
                "Inheritance : ['human', 'mammal']",
            }

        def _monkey_links():
            answers = _build_atom_answer(animal_base_handles.monkey)
            assert len(answers) == 5
            assert answers == {
                "Inheritance : ['monkey', 'mammal']",
                "Similarity : ['monkey', 'chimp']",
                "Similarity : ['chimp', 'monkey']",
                "Similarity : ['monkey', 'human']",
                "Similarity : ['human', 'monkey']",
            }

        def _chimp_links():
            answers = _build_atom_answer(animal_base_handles.chimp)
            assert len(answers) == 5
            assert answers == {
                "Inheritance : ['chimp', 'mammal']",
                "Similarity : ['monkey', 'chimp']",
                "Similarity : ['chimp', 'monkey']",
                "Similarity : ['chimp', 'human']",
                "Similarity : ['human', 'chimp']",
            }

        def _mammal_links():
            answers = _build_atom_answer(animal_base_handles.mammal)
            assert len(answers) == 5
            assert answers == {
                "Inheritance : ['mammal', 'animal']",
                "Inheritance : ['monkey', 'mammal']",
                "Inheritance : ['chimp', 'mammal']",
                "Inheritance : ['human', 'mammal']",
                "Inheritance : ['rhino', 'mammal']",
            }

        def _ent_links():
            answers = _build_atom_answer(animal_base_handles.ent)
            assert len(answers) == 3
            assert answers == {
                "Inheritance : ['ent', 'plant']",
                "Similarity : ['human', 'ent']",
                "Similarity : ['ent', 'human']",
            }

        def _animal_links():
            answers = _build_atom_answer(animal_base_handles.animal)
            assert len(answers) == 3
            assert answers == {
                "Inheritance : ['mammal', 'animal']",
                "Inheritance : ['reptile', 'animal']",
                "Inheritance : ['earthworm', 'animal']",
            }

        def _reptile_links():
            answers = _build_atom_answer(animal_base_handles.reptile)
            assert len(answers) == 3
            assert answers == {
                "Inheritance : ['dinosaur', 'reptile']",
                "Inheritance : ['snake', 'reptile']",
                "Inheritance : ['reptile', 'animal']",
            }

        def _dinosaur_links():
            answers = _build_atom_answer(animal_base_handles.dinosaur)
            assert len(answers) == 2
            assert answers == {
                "Inheritance : ['dinosaur', 'reptile']",
                "Inheritance : ['triceratops', 'dinosaur']",
            }

        def _triceratops_links():
            answers = _build_atom_answer(animal_base_handles.triceratops)
            assert len(answers) == 3
            assert answers == {
                "Similarity : ['rhino', 'triceratops']",
                "Similarity : ['triceratops', 'rhino']",
                "Inheritance : ['triceratops', 'dinosaur']",
            }

        def _rhino_links():
            answers = _build_atom_answer(animal_base_handles.rhino)
            assert len(answers) == 3
            assert answers == {
                "Similarity : ['rhino', 'triceratops']",
                "Similarity : ['triceratops', 'rhino']",
                "Inheritance : ['rhino', 'mammal']",
            }

        def _earthworm_links():
            answers = _build_atom_answer(animal_base_handles.earthworm)
            assert len(answers) == 3
            assert answers == {
                "Similarity : ['snake', 'earthworm']",
                "Similarity : ['earthworm', 'snake']",
                "Inheritance : ['earthworm', 'animal']",
            }

        def _snake_links():
            answers = _build_atom_answer(animal_base_handles.snake)
            assert len(answers) == 5
            assert answers == {
                "Similarity : ['snake', 'earthworm']",
                "Similarity : ['earthworm', 'snake']",
                "Inheritance : ['snake', 'reptile']",
                "Similarity : ['snake', 'vine']",
                "Similarity : ['vine', 'snake']",
            }

        def _vine_links():
            answers = _build_atom_answer(animal_base_handles.vine)
            assert len(answers) == 3
            assert answers == {
                "Inheritance : ['vine', 'plant']",
                "Similarity : ['snake', 'vine']",
                "Similarity : ['vine', 'snake']",
            }

        def _plant_links():
            answers = _build_atom_answer(animal_base_handles.plant)
            assert len(answers) == 2
            assert answers == {"Inheritance : ['vine', 'plant']", "Inheritance : ['ent', 'plant']"}

        def _similarity_inheritance_links():
            answers = _build_atom_answer(animal_base_handles.similarity_human_monkey)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_human_chimp)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_chimp_monkey)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_snake_earthworm)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_rhino_triceratops)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_snake_vine)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_human_ent)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_human_mammal)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_monkey_mammal)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_chimp_mammal)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_mammal_animal)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_reptile_animal)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_snake_reptile)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_dinosaur_reptile)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_triceratops_dinosaur)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_earthworm_animal)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_rhino_mammal)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_vine_plant)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.inheritance_ent_plant)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_monkey_human)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_chimp_human)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_monkey_chimp)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_earthworm_snake)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_triceratops_rhino)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_vine_snake)
            assert len(answers) == 0

            answers = _build_atom_answer(animal_base_handles.similarity_ent_human)
            assert len(answers) == 0

        _human_links()
        _monkey_links()
        _chimp_links()
        _mammal_links()
        _ent_links()
        _animal_links()
        _reptile_links()
        _dinosaur_links()
        _triceratops_links()
        _rhino_links()
        _earthworm_links()
        _snake_links()
        _vine_links()
        _plant_links()
        _similarity_inheritance_links()

    def test_get_links_with_filters(self, das):
        def _build_atom_answer(handle: str, **filters) -> dict:
            cursor = das.get_traversal_cursor(handle)
            links = cursor.get_links(**filters)
            return simplify_links(links, das)

        def _human_links():
            answers = _build_atom_answer(animal_base_handles.human, link_type='Similarity')
            assert answers == {
                "Similarity : ['human', 'chimp']",
                "Similarity : ['human', 'monkey']",
                "Similarity : ['human', 'ent']",
                "Similarity : ['ent', 'human']",
                "Similarity : ['monkey', 'human']",
                "Similarity : ['chimp', 'human']",
            }
            answers = _build_atom_answer(
                animal_base_handles.human, link_type='Similarity', cursor_position=0
            )
            assert answers == {
                "Similarity : ['human', 'chimp']",
                "Similarity : ['human', 'monkey']",
                "Similarity : ['human', 'ent']",
            }
            answers = _build_atom_answer(
                animal_base_handles.human, link_type='Similarity', cursor_position=1
            )
            assert answers == {
                "Similarity : ['ent', 'human']",
                "Similarity : ['monkey', 'human']",
                "Similarity : ['chimp', 'human']",
            }
            answers = _build_atom_answer(
                animal_base_handles.human,
                link_type='Similarity',
                cursor_position=0,
                target_type='Concept',
            )
            assert answers == {
                "Similarity : ['human', 'chimp']",
                "Similarity : ['human', 'monkey']",
                "Similarity : ['human', 'ent']",
            }
            answers = _build_atom_answer(animal_base_handles.human, link_type='Fake')
            assert len(answers) == 0
            answers = _build_atom_answer(animal_base_handles.human, cursor_position=2)
            assert len(answers) == 0
            answers = _build_atom_answer(animal_base_handles.human, target_type='Fake')
            assert len(answers) == 0

            das.add_link(
                {
                    'type': 'Similarity',
                    'targets': [
                        {'type': 'Concept', 'name': 'human'},
                        {'type': 'Concept', 'name': 'snet'},
                    ],
                    'weight': 0.5,
                }
            )

            def my_filter(link) -> bool:
                if 'weight' in link:
                    return True
                return False

            answers = _build_atom_answer(
                animal_base_handles.human,
                link_type='Similarity',
                cursor_position=0,
                target_type='Concept',
                filter=my_filter,
            )
            assert answers == {"Similarity : ['human', 'snet']"}

        def _mammal_links():
            answers = _build_atom_answer(animal_base_handles.mammal, link_type='Inheritance')
            assert answers == {
                "Inheritance : ['mammal', 'animal']",
                "Inheritance : ['monkey', 'mammal']",
                "Inheritance : ['chimp', 'mammal']",
                "Inheritance : ['human', 'mammal']",
                "Inheritance : ['rhino', 'mammal']",
            }
            answers = _build_atom_answer(
                animal_base_handles.mammal, link_type='Inheritance', cursor_position=0
            )
            assert answers == {"Inheritance : ['mammal', 'animal']"}
            answers = _build_atom_answer(
                animal_base_handles.mammal, link_type='Inheritance', cursor_position=1
            )
            assert answers == {
                "Inheritance : ['monkey', 'mammal']",
                "Inheritance : ['chimp', 'mammal']",
                "Inheritance : ['human', 'mammal']",
                "Inheritance : ['rhino', 'mammal']",
            }
            answers = _build_atom_answer(
                animal_base_handles.mammal,
                link_type='Inheritance',
                cursor_position=1,
                target_type='Concept',
            )
            assert answers == {
                "Inheritance : ['monkey', 'mammal']",
                "Inheritance : ['chimp', 'mammal']",
                "Inheritance : ['human', 'mammal']",
                "Inheritance : ['rhino', 'mammal']",
            }
            answers = _build_atom_answer(animal_base_handles.mammal, link_type='Similarity')
            assert len(answers) == 0
            answers = _build_atom_answer(animal_base_handles.mammal, cursor_position=5)
            assert len(answers) == 0
            answers = _build_atom_answer(animal_base_handles.mammal, target_type='Snet')
            assert len(answers) == 0

            das.add_link(
                {
                    'type': 'Inheritance',
                    'targets': [
                        {'type': 'Fake', 'name': 'fake1'},
                        {'type': 'Concept', 'name': 'mammal'},
                    ],
                    'weight': 0.4,
                }
            )
            das.add_link(
                {
                    'type': 'Inheritance',
                    'targets': [
                        {'type': 'Fake', 'name': 'fake2'},
                        {'type': 'Concept', 'name': 'mammal'},
                    ],
                    'weight': 0.5,
                }
            )

            def my_filter(link) -> bool:
                if 'weight' in link and link['weight'] >= 0.5:
                    return True
                return False

            answers = _build_atom_answer(
                animal_base_handles.mammal,
                link_type='Inheritance',
                cursor_position=1,
                target_type='Fake',
                filter=my_filter,
            )
            assert answers == {"Inheritance : ['fake2', 'mammal']"}

        def _snake_links():
            answers = _build_atom_answer(animal_base_handles.snake, link_type='Similarity')
            assert answers == {
                "Similarity : ['snake', 'earthworm']",
                "Similarity : ['earthworm', 'snake']",
                "Similarity : ['snake', 'vine']",
                "Similarity : ['vine', 'snake']",
            }
            answers = _build_atom_answer(
                animal_base_handles.snake, link_type='Inheritance', cursor_position=0
            )
            assert answers == {
                "Inheritance : ['snake', 'reptile']",
            }
            answers = _build_atom_answer(
                animal_base_handles.snake, link_type='Inheritance', cursor_position=1
            )
            assert len(answers) == 0
            answers = _build_atom_answer(
                animal_base_handles.snake, link_type='Similarity', cursor_position=0
            )
            assert answers == {
                "Similarity : ['snake', 'earthworm']",
                "Similarity : ['snake', 'vine']",
            }
            answers = _build_atom_answer(
                animal_base_handles.snake, link_type='Similarity', cursor_position=1
            )
            assert answers == {
                "Similarity : ['earthworm', 'snake']",
                "Similarity : ['vine', 'snake']",
            }
            answers = _build_atom_answer(
                animal_base_handles.snake,
                link_type='Inheritance',
                cursor_position=0,
                target_type='Concept',
            )
            assert answers == {"Inheritance : ['snake', 'reptile']"}
            answers = _build_atom_answer(animal_base_handles.snake, link_type='Evaluation')
            assert len(answers) == 0
            answers = _build_atom_answer(animal_base_handles.snake, cursor_position=5)
            assert len(answers) == 0
            answers = _build_atom_answer(
                animal_base_handles.snake,
                link_type='Inheritance',
                cursor_position=0,
                target_type='Snet',
            )
            assert len(answers) == 0

            das.add_link(
                {
                    'type': 'Similarity',
                    'targets': [
                        {'type': 'Fake', 'name': 'fake1'},
                        {'type': 'Concept', 'name': 'snake'},
                    ],
                    'weight': 0.2,
                }
            )
            das.add_link(
                {
                    'type': 'Similarity',
                    'targets': [
                        {'type': 'Concept', 'name': 'snake'},
                        {'type': 'Fake', 'name': 'fake1'},
                    ],
                    'weight': 0.5,
                }
            )

            def my_filter(link) -> bool:
                if 'weight' in link and link['weight'] >= 0.5:
                    return True
                return False

            answers = _build_atom_answer(
                animal_base_handles.snake,
                link_type='Similarity',
                target_type='Fake',
                filter=my_filter,
            )
            assert answers == {"Similarity : ['snake', 'fake1']"}

        def _similarity_human_monkey_links():
            answers = _build_atom_answer(
                animal_base_handles.similarity_human_monkey, link_type='Similarity'
            )
            assert len(answers) == 0

            das.add_link(
                {
                    'type': 'Inheritance',
                    'targets': [
                        {'type': 'Fake', 'name': 'fake'},
                        {
                            'type': 'Similarity',
                            'targets': [
                                {'type': 'Concept', 'name': 'human'},
                                {'type': 'Concept', 'name': 'monkey'},
                            ],
                        },
                    ],
                }
            )

            cursor = das.get_traversal_cursor(animal_base_handles.similarity_human_monkey)
            links = cursor.get_links(link_type='Inheritance')
            answers = [link for link in links]
            assert len(answers) == 1

        _human_links()
        _mammal_links()
        _snake_links()
        _similarity_human_monkey_links()

    def test_get_neighbors(self, das):
        def _build_neighbors(handle: str) -> dict:
            cursor = das.get_traversal_cursor(handle)
            neighbors_iterator = cursor.get_neighbors()
            return [item for item in neighbors_iterator]

        def _human_neighbors():
            neighbors = _build_neighbors(animal_base_handles.human)
            assert das.get_atom(animal_base_handles.monkey) in neighbors
            assert das.get_atom(animal_base_handles.mammal) in neighbors
            assert das.get_atom(animal_base_handles.chimp) in neighbors
            assert das.get_atom(animal_base_handles.ent) in neighbors
            assert len(neighbors) == 4

        def _monkey_neighbors():
            neighbors = _build_neighbors(animal_base_handles.monkey)
            assert das.get_atom(animal_base_handles.human) in neighbors
            assert das.get_atom(animal_base_handles.mammal) in neighbors
            assert das.get_atom(animal_base_handles.chimp) in neighbors
            assert len(neighbors) == 3

        def _chimp_neighbors():
            neighbors = _build_neighbors(animal_base_handles.chimp)
            assert das.get_atom(animal_base_handles.human) in neighbors
            assert das.get_atom(animal_base_handles.mammal) in neighbors
            assert das.get_atom(animal_base_handles.monkey) in neighbors
            assert len(neighbors) == 3

        def _mammal_neighbors():
            neighbors = _build_neighbors(animal_base_handles.mammal)
            assert das.get_atom(animal_base_handles.animal) in neighbors
            assert das.get_atom(animal_base_handles.monkey) in neighbors
            assert das.get_atom(animal_base_handles.chimp) in neighbors
            assert das.get_atom(animal_base_handles.human) in neighbors
            assert das.get_atom(animal_base_handles.rhino) in neighbors
            assert len(neighbors) == 5

        def _ent_neighbors():
            neighbors = _build_neighbors(animal_base_handles.ent)
            assert das.get_atom(animal_base_handles.human) in neighbors
            assert das.get_atom(animal_base_handles.plant) in neighbors
            assert len(neighbors) == 2

        def _animal_neighbors():
            neighbors = _build_neighbors(animal_base_handles.animal)
            assert das.get_atom(animal_base_handles.mammal) in neighbors
            assert das.get_atom(animal_base_handles.reptile) in neighbors
            assert das.get_atom(animal_base_handles.earthworm) in neighbors
            assert len(neighbors) == 3

        def _reptile_neighbors():
            neighbors = _build_neighbors(animal_base_handles.reptile)
            assert das.get_atom(animal_base_handles.animal) in neighbors
            assert das.get_atom(animal_base_handles.dinosaur) in neighbors
            assert das.get_atom(animal_base_handles.snake) in neighbors
            assert len(neighbors) == 3

        def _dinosaur_neighbors():
            neighbors = _build_neighbors(animal_base_handles.dinosaur)
            assert das.get_atom(animal_base_handles.reptile) in neighbors
            assert das.get_atom(animal_base_handles.triceratops) in neighbors
            assert len(neighbors) == 2

        def _triceratops_neighbors():
            neighbors = _build_neighbors(animal_base_handles.triceratops)
            assert das.get_atom(animal_base_handles.dinosaur) in neighbors
            assert das.get_atom(animal_base_handles.rhino) in neighbors
            assert len(neighbors) == 2

        def _rhino_neighbors():
            neighbors = _build_neighbors(animal_base_handles.rhino)
            assert das.get_atom(animal_base_handles.mammal) in neighbors
            assert das.get_atom(animal_base_handles.triceratops) in neighbors
            assert len(neighbors) == 2

        def _earthworm_neighbors():
            neighbors = _build_neighbors(animal_base_handles.earthworm)
            assert das.get_atom(animal_base_handles.animal) in neighbors
            assert das.get_atom(animal_base_handles.snake) in neighbors
            assert len(neighbors) == 2

        def _snake_neighbors():
            neighbors = _build_neighbors(animal_base_handles.snake)
            assert das.get_atom(animal_base_handles.earthworm) in neighbors
            assert das.get_atom(animal_base_handles.reptile) in neighbors
            assert das.get_atom(animal_base_handles.vine) in neighbors
            assert len(neighbors) == 3

        def _vine_neighbors():
            neighbors = _build_neighbors(animal_base_handles.vine)
            assert das.get_atom(animal_base_handles.snake) in neighbors
            assert das.get_atom(animal_base_handles.plant) in neighbors
            assert len(neighbors) == 2

        def _plant_neighbors():
            neighbors = _build_neighbors(animal_base_handles.plant)
            assert das.get_atom(animal_base_handles.vine) in neighbors
            assert das.get_atom(animal_base_handles.ent) in neighbors
            assert len(neighbors) == 2

        def _similarity_inheritance_neighbors():
            answers = _build_neighbors(animal_base_handles.similarity_human_monkey)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_human_chimp)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_chimp_monkey)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_snake_earthworm)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_rhino_triceratops)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_snake_vine)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_human_ent)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_human_mammal)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_monkey_mammal)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_chimp_mammal)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_mammal_animal)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_reptile_animal)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_snake_reptile)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_dinosaur_reptile)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_triceratops_dinosaur)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_earthworm_animal)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_rhino_mammal)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_vine_plant)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.inheritance_ent_plant)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_monkey_human)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_chimp_human)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_monkey_chimp)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_earthworm_snake)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_triceratops_rhino)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_vine_snake)
            assert len(answers) == 0

            answers = _build_neighbors(animal_base_handles.similarity_ent_human)
            assert len(answers) == 0

        _human_neighbors()
        _monkey_neighbors()
        _chimp_neighbors()
        _mammal_neighbors()
        _ent_neighbors()
        _animal_neighbors()
        _reptile_neighbors()
        _dinosaur_neighbors()
        _triceratops_neighbors()
        _rhino_neighbors()
        _earthworm_neighbors()
        _snake_neighbors()
        _vine_neighbors()
        _plant_neighbors()
        _similarity_inheritance_neighbors()

    def test_get_neighbors_with_filters(self, das):
        def _build_neighbors(handle: str, **filters) -> dict:
            cursor = das.get_traversal_cursor(handle)
            neighbors_iterator = cursor.get_neighbors(**filters)
            ret = []
            for item in neighbors_iterator:
                ret.append(item)
            return ret

        def _human_neighbors():
            neighbors = _build_neighbors(animal_base_handles.human, link_type='Inheritance')
            assert das.get_atom(animal_base_handles.mammal) in neighbors
            assert len(neighbors) == 1

            neighbors = _build_neighbors(animal_base_handles.human, link_type='Similarity')
            assert das.get_atom(animal_base_handles.monkey) in neighbors
            assert das.get_atom(animal_base_handles.chimp) in neighbors
            assert das.get_atom(animal_base_handles.ent) in neighbors
            assert len(neighbors) == 3

            neighbors = _build_neighbors(animal_base_handles.human, target_type='Concept')
            assert das.get_atom(animal_base_handles.mammal) in neighbors
            assert das.get_atom(animal_base_handles.monkey) in neighbors
            assert das.get_atom(animal_base_handles.chimp) in neighbors
            assert das.get_atom(animal_base_handles.ent) in neighbors
            assert len(neighbors) == 4

            neighbors = _build_neighbors(
                animal_base_handles.human, link_type='Inheritance', target_type='Snet'
            )
            assert len(neighbors) == 0

            neighbors = _build_neighbors(
                animal_base_handles.human, link_type='Similarity', target_type='Snet'
            )
            assert len(neighbors) == 0

            das.add_link(
                {
                    'type': 'Similarity',
                    'targets': [
                        {'type': 'Concept', 'name': 'human'},
                        {'type': 'Fake', 'name': 'fake-h', 'weight': 0.7},
                    ],
                }
            )
            das.add_link(
                {
                    'type': 'Similarity',
                    'targets': [
                        {'type': 'Fake', 'name': 'fake-h', 'weight': 0.3},
                        {'type': 'Concept', 'name': 'human'},
                    ],
                }
            )
            das.add_link(
                {
                    'type': 'Inheritance',
                    'targets': [
                        {
                            'type': 'Fake',
                            'name': 'fake-h2',
                            'weight': 0.3,
                        },
                        {
                            'type': 'Fake',
                            'name': 'fake-h3',
                            'weight': 0.3,
                        },
                        {
                            'type': 'Fake',
                            'name': 'fake-h4',
                            'weight': 1.3,
                        },
                        {'type': 'Concept', 'name': 'human'},
                    ],
                }
            )

            def my_filter(target) -> bool:
                if 'weight' in target and target['weight'] >= 1:
                    return True
                return False

            fake_h = AtomDB.node_handle('Fake', 'fake-h')
            fake_h2 = AtomDB.node_handle('Fake', 'fake-h2')
            fake_h3 = AtomDB.node_handle('Fake', 'fake-h3')
            fake_h4 = AtomDB.node_handle('Fake', 'fake-h4')

            neighbors = _build_neighbors(animal_base_handles.human)
            assert das.get_atom(animal_base_handles.mammal) in neighbors
            assert das.get_atom(animal_base_handles.monkey) in neighbors
            assert das.get_atom(animal_base_handles.chimp) in neighbors
            assert das.get_atom(animal_base_handles.ent) in neighbors
            assert das.get_atom(fake_h) in neighbors
            assert das.get_atom(fake_h2) in neighbors
            assert das.get_atom(fake_h3) in neighbors
            assert das.get_atom(fake_h4) in neighbors
            assert len(neighbors) == 8

            neighbors = _build_neighbors(
                animal_base_handles.human, link_type='Similarity', target_type='Fake'
            )
            assert das.get_atom(fake_h) in neighbors
            assert len(neighbors) == 1

            neighbors = _build_neighbors(
                animal_base_handles.human,
                link_type='Inheritance',
                target_type='Fake',
                filters=(None, my_filter),
            )
            assert len(neighbors) == 1

            neighbors = _build_neighbors(
                animal_base_handles.human,
                link_type='Inheritance',
                target_type='Fake',
                filters=None,
            )

            assert len(neighbors) == 3

            with pytest.raises(ValueError):
                _build_neighbors(
                    animal_base_handles.human,
                    link_type='Inheritance',
                    target_type='Fake',
                    filters=my_filter,
                )

        def _vine_neighbors():
            neighbors = _build_neighbors(animal_base_handles.vine, link_type='Similarity')
            assert das.get_atom(animal_base_handles.snake) in neighbors
            assert len(neighbors) == 1

            neighbors = _build_neighbors(animal_base_handles.vine, link_type='Inheritance')
            assert das.get_atom(animal_base_handles.plant) in neighbors
            assert len(neighbors) == 1

            fake_v1 = AtomDB.node_handle('Fake', 'fake-v1')
            fake_v2 = AtomDB.node_handle('Fake', 'fake-v2')

            das.add_link(
                {
                    'type': 'Inheritance',
                    'targets': [
                        {
                            'type': 'Fake',
                            'name': 'fake-v1',
                            'weight': 1,
                        },
                        {'type': 'Concept', 'name': 'vine'},
                    ],
                }
            )
            das.add_link(
                {
                    'type': 'Similarity',
                    'targets': [
                        {'type': 'Concept', 'name': 'vine'},
                        {
                            'type': 'Fake',
                            'name': 'fake-v2',
                            'weight': 0.7,
                        },
                    ],
                }
            )
            das.add_link(
                {
                    'type': 'Similarity',
                    'targets': [
                        {
                            'type': 'Fake',
                            'name': 'fake-v2',
                            'weight': 0.3,
                        },
                        {'type': 'Concept', 'name': 'vine'},
                    ],
                }
            )

            neighbors = _build_neighbors(animal_base_handles.vine, link_type='Inheritance')
            assert das.get_atom(animal_base_handles.plant) in neighbors
            assert das.get_atom(fake_v1) in neighbors
            assert len(neighbors) == 2

            neighbors = _build_neighbors(animal_base_handles.vine, link_type='Similarity')
            assert das.get_atom(animal_base_handles.snake) in neighbors
            assert das.get_atom(fake_v2) in neighbors
            assert len(neighbors) == 2

            neighbors = _build_neighbors(
                animal_base_handles.vine, link_type='Similarity', target_type='Concept'
            )
            assert das.get_atom(animal_base_handles.snake) in neighbors
            assert len(neighbors) == 1

            neighbors = _build_neighbors(
                animal_base_handles.vine, link_type='Inheritance', target_type='Fake'
            )
            assert das.get_atom(fake_v1) in neighbors
            assert len(neighbors) == 1

            neighbors = _build_neighbors(
                animal_base_handles.vine, link_type='Similarity', target_type='Fake'
            )
            assert das.get_atom(fake_v2) in neighbors
            assert len(neighbors) == 1

            def my_filter(target) -> bool:
                if 'weight' in target and target['weight'] >= 1:
                    return True
                return False

            neighbors = _build_neighbors(
                animal_base_handles.vine, link_type='Similarity', filters=(None, my_filter)
            )
            assert len(neighbors) == 0

            neighbors = _build_neighbors(
                animal_base_handles.vine, link_type='Inheritance', filters=(None, my_filter)
            )
            assert das.get_atom(fake_v1) in neighbors
            assert len(neighbors) == 1

        def _inheritance_dinosaur_reptile():
            neighbors = _build_neighbors(animal_base_handles.inheritance_dinosaur_reptile)
            assert len(neighbors) == 0

            fake_dr1 = AtomDB.node_handle('Fake', 'fake-dr1')
            fake_dr2 = AtomDB.node_handle('Fake', 'fake-dr2')

            das.add_link(
                {
                    'type': 'Inheritance',
                    'targets': [
                        {'type': 'Fake', 'name': 'fake-dr1'},
                        {
                            'type': 'Inheritance',
                            'targets': [
                                {'type': 'Concept', 'name': 'dinosaur'},
                                {'type': 'Concept', 'name': 'reptile'},
                            ],
                        },
                    ],
                    'weight': 1,
                }
            )
            das.add_link(
                {
                    'type': 'Similarity',
                    'targets': [
                        {
                            'type': 'Inheritance',
                            'targets': [
                                {'type': 'Concept', 'name': 'dinosaur'},
                                {'type': 'Concept', 'name': 'reptile'},
                            ],
                        },
                        {'type': 'Fake', 'name': 'fake-dr2'},
                    ],
                    'weight': 0.7,
                }
            )
            das.add_link(
                {
                    'type': 'Similarity',
                    'targets': [
                        {'type': 'Fake', 'name': 'fake-dr2'},
                        {
                            'type': 'Inheritance',
                            'targets': [
                                {'type': 'Concept', 'name': 'dinosaur'},
                                {'type': 'Concept', 'name': 'reptile'},
                            ],
                        },
                    ],
                    'weight': 0.3,
                }
            )

            neighbors = _build_neighbors(animal_base_handles.inheritance_dinosaur_reptile)
            assert das.get_atom(fake_dr1) in neighbors
            assert das.get_atom(fake_dr2) in neighbors
            assert len(neighbors) == 2

            neighbors = _build_neighbors(
                animal_base_handles.inheritance_dinosaur_reptile, link_type='Similarity'
            )
            assert das.get_atom(fake_dr2) in neighbors
            assert len(neighbors) == 1

            neighbors = _build_neighbors(
                animal_base_handles.inheritance_dinosaur_reptile, link_type='Inheritance'
            )
            assert das.get_atom(fake_dr1) in neighbors
            assert len(neighbors) == 1

            neighbors = _build_neighbors(
                animal_base_handles.inheritance_dinosaur_reptile,
                link_type='Inheritance',
                target_type='Fake',
            )
            assert das.get_atom(fake_dr1) in neighbors
            assert len(neighbors) == 1

            neighbors = _build_neighbors(
                animal_base_handles.inheritance_dinosaur_reptile,
                link_type='Inheritance',
                target_type='Concept',
            )
            assert len(neighbors) == 0

            neighbors = _build_neighbors(
                animal_base_handles.inheritance_dinosaur_reptile, target_type='Concept'
            )
            assert len(neighbors) == 0

            neighbors = _build_neighbors(
                animal_base_handles.inheritance_dinosaur_reptile, target_type='Fake'
            )
            assert das.get_atom(fake_dr1) in neighbors
            assert das.get_atom(fake_dr2) in neighbors
            assert len(neighbors) == 2

        _human_neighbors()
        _vine_neighbors()
        _inheritance_dinosaur_reptile()

    @pytest.mark.parametrize("n", range(10))
    def test_follow_link(self, das, n):
        def _mammal():
            cursor = das.get_traversal_cursor(animal_base_handles.mammal)
            assert cursor.get()['name'] == 'mammal'

            cursor.follow_link()
            current_cursor = cursor.get()['name']
            assert current_cursor in ('monkey', 'chimp', 'human', 'animal', 'rhino')

            cursor.follow_link()
            previous_cursor = current_cursor
            current_cursor = cursor.get()['name']
            if previous_cursor == 'monkey':
                assert current_cursor in ('mammal', 'chimp', 'human')
            elif previous_cursor == 'chimp':
                assert current_cursor in ('mammal', 'monkey', 'human')
            elif previous_cursor == 'human':
                assert current_cursor in ('mammal', 'monkey', 'chimp', 'ent')
            elif previous_cursor == 'animal':
                assert current_cursor in ('mammal', 'reptile', 'earthworm')
            elif previous_cursor == 'rhino':
                assert current_cursor in ('mammal', 'triceratops')

        def _earthworm():
            cursor = das.get_traversal_cursor(animal_base_handles.earthworm)
            assert cursor.get()['name'] == 'earthworm'

            cursor.follow_link()
            current_cursor = cursor.get()['name']
            assert current_cursor in ('animal', 'snake')

            cursor.follow_link()
            previous_cursor = current_cursor
            current_cursor = cursor.get()['name']

            if previous_cursor == 'animal':
                assert current_cursor in ('mammal', 'reptile', 'earthworm')
            elif previous_cursor == 'snake':
                assert current_cursor in ('earthworm', 'reptile', 'vine')

        _mammal()
        _earthworm()

    @pytest.mark.parametrize("n", range(10))
    def test_follow_link_with_filters(self, das, n):
        def _mammal():
            cursor = das.get_traversal_cursor(animal_base_handles.mammal)
            assert cursor.get()['name'] == 'mammal'

            cursor.follow_link(link_type='Similarity')
            assert cursor.get()['name'] == 'mammal'

            cursor.follow_link(link_type='Inheritance')
            current_cursor = cursor.get()['name']
            assert current_cursor in ('monkey', 'chimp', 'human', 'animal', 'rhino')

            cursor.follow_link(link_type='Inheritance', target_type='Concept')
            previous_cursor = current_cursor
            current_cursor = cursor.get()['name']
            if previous_cursor == 'monkey':
                assert current_cursor in ('mammal', 'chimp')
            elif previous_cursor == 'chimp':
                assert current_cursor in ('mammal', 'monkey', 'human')
            elif previous_cursor == 'human':
                assert current_cursor in ('mammal', 'monkey', 'chimp', 'ent')
            elif previous_cursor == 'animal':
                assert current_cursor in ('mammal', 'reptile', 'earthworm')
            elif previous_cursor == 'rhino':
                assert current_cursor in ('mammal', 'triceratops')

            cursor.follow_link(link_type='Inheritance', target_type='Fake')
            previous_cursor = current_cursor
            current_cursor = cursor.get()['name']
            assert previous_cursor == current_cursor

        _mammal()

    def test_goto(self, das):
        cursor = das.get_traversal_cursor(animal_base_handles.human)
        cursor.get()['name'] == 'human'

        cursor.goto(animal_base_handles.ent)
        assert cursor.get()['name'] == 'ent'

        with pytest.raises(AtomDoesNotExist):
            cursor.goto('snet')
