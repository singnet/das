from collections import deque
from unittest import mock

import pytest

from hyperon_das.cache.iterators import (
    BaseLinksIterator,
    ListIterator,
    LocalIncomingLinks,
    ProductIterator,
    RemoteIncomingLinks,
    TraverseLinksIterator,
    TraverseNeighborsIterator,
)
from hyperon_das.utils import Assignment


class TestListIterator:
    def test_list_iterator(self):
        iterator = ListIterator(None)
        for _element in iterator:
            assert False

        iterator = ListIterator([])
        for _element in iterator:
            assert False

        iterator = ListIterator(
            [
                ([{"id": 1}], Assignment()),
            ]
        )
        expected = [1]
        count = 0
        for element in iterator:
            assert element[0][0]["id"] == expected[count]
            count += 1
        assert count == len(expected)

        iterator = ListIterator(
            [
                ([{"id": 1}], Assignment()),
                ([{"id": 2}], Assignment()),
            ]
        )
        expected = [1, 2]
        count = 0
        for element in iterator:
            assert element[0][0]["id"] == expected[count]
            count += 1
        assert count == len(expected)

        iterator = ListIterator(
            [
                ([{"id": 1}], Assignment()),
                ([{"id": 2}], Assignment()),
                ([{"id": 2}], Assignment()),
            ]
        )
        expected = [1, 2, 2]
        count = 0
        for element in iterator:
            assert element[0][0]["id"] == expected[count]
            count += 1
        assert count == len(expected)

        iterator = ListIterator(
            [
                ([{"id": 1}], Assignment()),
            ]
        )
        assert not iterator.is_empty()
        iterator = ListIterator([None])
        assert not iterator.is_empty()
        iterator = ListIterator([])
        assert iterator.is_empty()
        iterator = ListIterator(None)
        assert iterator.is_empty()


class TestProductIterator:
    def test_product_iterator(self):
        ln = None
        l0 = []
        l1 = [1, 2, 3]
        l2 = [4]
        l3 = [5, 6]
        l4 = [7, 8]

        li1 = ListIterator(l1)
        li3 = ListIterator(l3)
        iterator = ProductIterator([li1, li3])
        assert not iterator.is_empty()
        assert iterator.get() == (1, 5)
        expected = [(1, 5), (1, 6), (2, 5), (2, 6), (3, 5), (3, 6)]
        count = 0
        for element in iterator:
            assert element == expected[count]
            assert iterator.get() == expected[count]
            count += 1
        assert not iterator.is_empty()
        with pytest.raises(StopIteration):
            assert iterator.get()

        li3 = ListIterator(l3)
        li4 = ListIterator(l4)
        iterator = ProductIterator([li3, li4])
        expected = [(5, 7), (5, 8), (6, 7), (6, 8)]
        count = 0
        for element in iterator:
            assert element == expected[count]
            count += 1
        assert not iterator.is_empty()

        li1 = ListIterator(l1)
        li2 = ListIterator(l2)
        li3 = ListIterator(l3)
        iterator = ProductIterator([li1, li2, li3])
        expected = [
            (1, 4, 5),
            (1, 4, 6),
            (2, 4, 5),
            (2, 4, 6),
            (3, 4, 5),
            (3, 4, 6),
        ]
        count = 0
        for element in iterator:
            assert element == expected[count]
            count += 1
        with pytest.raises(StopIteration):
            assert iterator.get()

        for arg in [[ln, l1], [ln, l1, l2], [ln]]:
            iterator = ProductIterator([ListIterator(v) for v in arg])
            assert iterator.is_empty()
            for _element in iterator:
                assert False
            assert iterator.is_empty()

        for arg in [[l0, l1], [l0, l1, l2], [l0]]:
            iterator = ProductIterator([ListIterator(v) for v in arg])
            assert iterator.is_empty()
            for _element in iterator:
                assert False
            assert iterator.is_empty()


class ConcreteBaseLinksIterator(BaseLinksIterator):
    def get_current_value(self):
        return "current_value"

    def get_fetch_data(self, **kwargs):
        return 2024, []

    def get_fetch_data_kwargs(self):
        return {"fetch_data_kwargs": True}

    def get_next_value(self):
        return "next_value"


class TestBaseLinksIterator:
    def test_init(self):
        source = ListIterator([1, 2, 3])
        backend = mock.MagicMock()
        chunk_size = 500
        cursor = 2024
        iterator = ConcreteBaseLinksIterator(
            source, backend=backend, chunk_size=chunk_size, cursor=cursor
        )
        assert iterator.source == source
        assert iterator.backend == backend
        assert iterator.chunk_size == chunk_size
        assert iterator.cursor == cursor
        assert iterator.iterator == source
        assert iterator.current_value == iterator.get_current_value()
        assert iterator.semaphore._value == 1

    def test_next(self):
        source = ListIterator([1, 2, 3])
        iterator = ConcreteBaseLinksIterator(source)
        iterator.get_next_value = mock.Mock(side_effect=[StopIteration()])
        with pytest.raises(StopIteration):
            next(iterator)
        assert iterator.iterator is None
        assert iterator.current_value is None
        iterator.get_next_value.assert_called_once()

    def test_fetch_data(self):
        source = ListIterator([1, 2, 3])
        iterator = ConcreteBaseLinksIterator(source, cursor=1)
        iterator.get_fetch_data_kwargs = mock.MagicMock(return_value={})
        iterator.get_fetch_data = mock.MagicMock(return_value=(0, []))
        iterator._fetch_data()
        iterator.get_fetch_data_kwargs.assert_called_once()
        iterator.get_fetch_data.assert_called_once()
        assert iterator.cursor == 0

    def test_refresh_iterator(self):
        source = ListIterator([1, 2, 3])
        iterator = ConcreteBaseLinksIterator(source, cursor=1)
        iterator.get_current_value = mock.MagicMock(return_value="current_value")
        iterator._refresh_iterator()

        iterator.get_current_value.assert_called_once()
        assert iterator.source.source == ListIterator(list(iterator.buffer_queue)).source
        assert iterator.iterator == iterator.source
        assert iterator.current_value == "current_value"
        assert iterator.buffer_queue == deque()

    def test_is_empty(self):
        iterator = ConcreteBaseLinksIterator(ListIterator([1, 2, 3]))
        assert iterator.is_empty() is False
        iterator = ConcreteBaseLinksIterator(ListIterator([]))
        assert iterator.is_empty() is True


class TestLocalIncomingLinks:
    @pytest.fixture
    def backend(self):
        backend = mock.MagicMock()
        backend.get_atom.side_effect = lambda x, targets_document=None: {"handle": x}
        return backend

    def test_get_next_value(self, backend):
        iterator = LocalIncomingLinks(ListIterator([1, 2, 3]), backend=backend)

        iterator.get_next_value()
        assert iterator.current_value == backend.get_atom(1)

        iterator.get_next_value()
        assert iterator.current_value == backend.get_atom(2)

        iterator.get_next_value()
        assert iterator.current_value == backend.get_atom(3)

        with pytest.raises(StopIteration):
            iterator.get_next_value()

    def test_get_current_value(self, backend):
        iterator = LocalIncomingLinks(ListIterator([1, 2, 3]), backend=backend)

        assert iterator.get_current_value() == backend.get_atom(1)

        iterator.get_next_value()
        assert iterator.get_current_value() == backend.get_atom(1)

        iterator.get_next_value()
        assert iterator.get_current_value() == backend.get_atom(2)

        iterator.get_next_value()
        assert iterator.get_current_value() == backend.get_atom(3)

        iterator = LocalIncomingLinks(ListIterator([]), backend=backend)
        assert iterator.is_empty() is True

    def test_get_fetch_data_kwargs(self, backend):
        iterator = LocalIncomingLinks(ListIterator([1, 2, 3]), backend=backend)
        assert iterator.get_fetch_data_kwargs() == {
            "handles_only": True,
            "cursor": iterator.cursor,
            "chunk_size": iterator.chunk_size,
        }

    def test_get_fetch_data(self, backend):
        iterator = LocalIncomingLinks(ListIterator([1, 2, 3]), backend=backend)
        kwargs = {"param1": "value1", "param2": "value2"}
        result = iterator.get_fetch_data(**kwargs)
        assert result == backend.get_incoming_links(iterator.atom_handle, **kwargs)


class TestRemoteIncomingLinks:
    def test_get_next_value(self):
        source = ListIterator(
            [
                {"handle": "link1"},
                {"handle": "link2"},
                {"handle": "link3"},
                {"handle": "link4"},
            ]
        )
        iterator = RemoteIncomingLinks(source)
        iterator.returned_handles = set(["link2", "link3"])

        iterator.get_next_value()
        assert iterator.current_value == {"handle": "link1"}

        iterator.get_next_value()
        assert iterator.current_value == {"handle": "link4"}

    def test_get_current_value(self):
        source = ListIterator(
            [
                {"handle": "link1"},
                {"handle": "link2"},
            ]
        )
        iterator = RemoteIncomingLinks(source)

        iterator.get_next_value()
        assert iterator.get_current_value() == {"handle": "link1"}

        iterator.get_next_value()
        assert iterator.get_current_value() == {"handle": "link2"}

        iterator = RemoteIncomingLinks(ListIterator([]))
        assert iterator.is_empty() is True

    def test_get_fetch_data_kwargs(self):
        source = ListIterator(
            [
                {"handle": "link1"},
                {"handle": "link2"},
            ]
        )
        iterator = RemoteIncomingLinks(source, atom_handle="atom1", targets_document=True)
        kwargs = iterator.get_fetch_data_kwargs()
        assert kwargs == {
            "cursor": iterator.cursor,
            "chunk_size": iterator.chunk_size,
            "targets_document": iterator.targets_document,
        }

    def test_get_fetch_data(self):
        backend = mock.MagicMock()
        backend.get_incoming_links.return_value = (
            123,
            [{"handle": "link1"}, {"handle": "link2"}],
        )
        source = ListIterator([])
        iterator = RemoteIncomingLinks(source, atom_handle="atom1")
        iterator.backend = backend
        result = iterator.get_fetch_data(cursor=0, chunk_size=100)
        assert result == (123, [{"handle": "link1"}, {"handle": "link2"}])
        backend.get_incoming_links.assert_called_once_with("atom1", cursor=0, chunk_size=100)


class TestTraverseLinksIterator:
    @pytest.fixture
    def incoming_links(self):
        source = ListIterator(["link1", "link2", "link3"])
        backend = mock.Mock()
        targets_document = True
        backend.get_atom.side_effect = lambda handle, targets_document=targets_document: (
            {
                "handle": handle,
                "named_type": f"Type{handle[-1]}",
                "targets": ["node11", f"node{handle[-1]}2"],
            },
            [
                {"handle": "node11", "named_type": "Type2"},
                {
                    "handle": f"node{handle[-1]}2",
                    "named_type": f"Type{int(handle[-1]) + 1}",
                },
            ],
        )
        return LocalIncomingLinks(source=source, backend=backend, targets_document=targets_document)

    def test_empty_source(self):
        source = LocalIncomingLinks(ListIterator([]))
        iterator = TraverseLinksIterator(source)
        assert iterator.is_empty() is True
        with pytest.raises(StopIteration):
            next(iterator)

    def test_no_filters(self, incoming_links):
        iterator = TraverseLinksIterator(source=incoming_links)

        assert iterator.is_empty() is False
        assert next(iterator) == {
            "handle": "link1",
            "named_type": "Type1",
            "targets": ["node11", "node12"],
        }
        assert next(iterator) == {
            "handle": "link2",
            "named_type": "Type2",
            "targets": ["node11", "node22"],
        }
        assert next(iterator) == {
            "handle": "link3",
            "named_type": "Type3",
            "targets": ["node11", "node32"],
        }
        with pytest.raises(StopIteration):
            next(iterator)
        assert iterator.is_empty() is True

    def test_with_filters(self, incoming_links):
        iterator = TraverseLinksIterator(
            source=incoming_links, link_type="Type2", target_type="Type3"
        )

        assert iterator.is_empty() is False
        assert next(iterator) == {
            "handle": "link2",
            "named_type": "Type2",
            "targets": ["node11", "node22"],
        }
        with pytest.raises(StopIteration):
            next(iterator)
        assert iterator.is_empty() is True

    def test_cursor_position(self, incoming_links):
        iterator = TraverseLinksIterator(incoming_links, cursor_position=0, cursor="node11")
        assert iterator.is_empty() is False
        assert next(iterator) == {
            "handle": "link1",
            "named_type": "Type1",
            "targets": ["node11", "node12"],
        }
        assert next(iterator) == {
            "handle": "link2",
            "named_type": "Type2",
            "targets": ["node11", "node22"],
        }
        assert next(iterator) == {
            "handle": "link3",
            "named_type": "Type3",
            "targets": ["node11", "node32"],
        }
        with pytest.raises(StopIteration):
            next(iterator)
        assert iterator.is_empty() is True

    def test_custom_filter(self, incoming_links):
        def custom_filter(link):
            return link["named_type"] == "Type3"

        iterator = TraverseLinksIterator(incoming_links, filter=custom_filter)

        assert iterator.is_empty() is False
        assert next(iterator) == {
            "handle": "link3",
            "named_type": "Type3",
            "targets": ["node11", "node32"],
        }
        with pytest.raises(StopIteration):
            next(iterator)
        assert iterator.is_empty() is True

    def test_targets_only(self, incoming_links):
        iterator = TraverseLinksIterator(incoming_links, targets_only=True)
        assert iterator.is_empty() is False
        assert next(iterator) == [
            {"handle": "node11", "named_type": "Type2"},
            {"handle": "node12", "named_type": "Type2"},
        ]
        assert next(iterator) == [
            {"handle": "node11", "named_type": "Type2"},
            {"handle": "node22", "named_type": "Type3"},
        ]
        assert next(iterator) == [
            {"handle": "node11", "named_type": "Type2"},
            {"handle": "node32", "named_type": "Type4"},
        ]
        with pytest.raises(StopIteration):
            next(iterator)
        assert iterator.is_empty() is True


class TestTraverseNeighborsIterator:
    @pytest.fixture
    def traverse_links_iterator(self):
        source = ListIterator(["link1", "link2", "link3"])
        backend = mock.Mock()
        targets_document = True
        backend.get_atom.side_effect = lambda handle, targets_document=targets_document: (
            {
                "handle": handle,
                "named_type": f"Type{handle[-1]}",
                "targets": ["node11", f"node{handle[-1]}2"],
            },
            [
                {"handle": "node11", "named_type": "Type2"},
                {
                    "handle": f"node{handle[-1]}2",
                    "named_type": f"Type{int(handle[-1]) + 1}",
                },
            ],
        )
        incoming_links = LocalIncomingLinks(
            source=source, backend=backend, targets_document=targets_document
        )

        return TraverseLinksIterator(incoming_links, targets_only=True, cursor="node11")

    def test_init(self, traverse_links_iterator):
        iterator = TraverseNeighborsIterator(source=traverse_links_iterator)
        assert iterator.source == traverse_links_iterator
        assert iterator.buffered_answer is not None
        assert iterator.cursor == traverse_links_iterator.cursor
        assert iterator.target_type == traverse_links_iterator.target_type
        assert iterator.visited_neighbors == ["node12"]
        assert iterator.iterator == traverse_links_iterator
        assert iterator.current_value == {"handle": "node12", "named_type": "Type2"}

    def test_next_with_buffered_answer(self):
        iterator = TraverseNeighborsIterator(source=mock.Mock())
        iterator.buffered_answer = ListIterator([1, 2, 3])
        assert next(iterator) == 1
        assert next(iterator) == 2
        assert next(iterator) == 3

    def test_next_without_buffered_answer(self, traverse_links_iterator):
        iterator = TraverseNeighborsIterator(source=traverse_links_iterator)
        assert next(iterator) == {"handle": "node12", "named_type": "Type2"}
        assert next(iterator) == {"handle": "node22", "named_type": "Type3"}
        assert next(iterator) == {"handle": "node32", "named_type": "Type4"}
        with pytest.raises(StopIteration):
            next(iterator)

    def test_process_targets(self, traverse_links_iterator):
        iterator = TraverseNeighborsIterator(source=traverse_links_iterator)
        targets = [
            {"handle": "node11", "named_type": "Type2"},
            {"handle": "node22", "named_type": "Type3"},
        ]
        answer, match_found = iterator._process_targets(targets)
        assert answer == [{"handle": "node22", "named_type": "Type3"}]
        assert match_found is True

    def test_filter(self, traverse_links_iterator):
        iterator = TraverseNeighborsIterator(source=traverse_links_iterator)

        target = {"handle": "node11", "named_type": "Type2"}
        assert iterator._filter(target) is False

        target = {"handle": "node22", "named_type": "Type3"}
        assert iterator._filter(target) is True

    def test_is_empty(self):
        iterator = TraverseNeighborsIterator(
            source=TraverseLinksIterator(LocalIncomingLinks(ListIterator([])))
        )
        iterator.current_value = None
        assert iterator.is_empty() is True

        iterator.current_value = {"handle": 1}
        assert iterator.is_empty() is False
