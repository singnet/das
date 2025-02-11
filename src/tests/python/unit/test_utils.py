import pytest

from hyperon_das.exceptions import InvalidAssignment
from hyperon_das.utils import (
    Assignment,
    QueryAnswer,
    compare_major_versions,
    compare_minor_versions,
    compare_patch_versions,
    get_version_components,
)


def _build_assignment(mappings):
    answer = Assignment()
    for label, value in mappings:
        answer.assign(label, value)
    return answer


def _check_merge(l1, l2, expected_list, flag):
    a1 = _build_assignment(l1)
    a2 = _build_assignment(l2)
    expected = _build_assignment(expected_list)
    assert a1.merge(a2) == flag
    if flag:
        a1.freeze()
        expected.freeze()
        assert a1.__eq__(expected)


class TestAssignment:
    def test_basics(self):
        va1 = Assignment()
        assert va1.assign("v1", "1")
        assert va1.assign("v2", "2")
        with pytest.raises(InvalidAssignment):
            va1.assign("v3", None)
        with pytest.raises(InvalidAssignment):
            va1.assign(None, "3")
        assert not va1.assign("v3", "2", parameters={"no_overload": True})
        assert va1.assign("v3", "2", parameters={"no_overload": False})
        assert va1.assign("v3", "2")
        assert va1.hashcode == 0
        assert va1.freeze()
        assert va1.hashcode != 0
        with pytest.raises(InvalidAssignment):
            assert va1.assign("v4", "4")
        a1 = _build_assignment([("v1", "1"), ("v2", "2")])
        a2 = _build_assignment([("v2", "2"), ("v1", "1")])
        a3 = _build_assignment([("v1", "1"), ("v2", "1")])
        with pytest.raises(AssertionError):
            a1.__eq__(a2)
        assert a1.freeze()
        assert a2.freeze()
        assert a1.__eq__(a2)
        assert a2.__eq__(a1)
        assert a3.freeze()
        assert not a1.__eq__(a3)
        assert not a3.__eq__(a1)

    def test_assignment_sets(self):
        va1 = Assignment()
        va2 = Assignment()
        va3 = Assignment()
        va1.assign("v1", "1")
        va1.assign("v2", "2")
        va2.assign("v2", "2")
        va2.assign("v1", "1")
        va3.assign("v1", "2")
        va3.assign("v2", "1")

        with pytest.raises(Exception):
            s1 = set([va1, va2])
        with pytest.raises(Exception):
            s2 = set([va1, va3])

        va1.freeze()
        va2.freeze()
        va3.freeze()
        s1 = set([va1, va2])
        s2 = set([va1, va3])
        assert len(s1) == 1
        assert len(s2) == 2
        assert va1 in s1 and va2 in s1 and va3 not in s1
        assert va1 in s2 and va2 in s2 and va3 in s2

    def test_merge(self):
        _check_merge(
            [("v1", "1"), ("v2", "2")],
            [("v3", "3"), ("v4", "4")],
            [("v1", "1"), ("v2", "2"), ("v3", "3"), ("v4", "4")],
            True,
        )

        _check_merge(
            [("v1", "1"), ("v2", "2")],
            [("v1", "1"), ("v5", "5")],
            [("v1", "1"), ("v2", "2"), ("v5", "5")],
            True,
        )

        _check_merge([("v1", "1"), ("v2", "2")], [("v1", "2"), ("v5", "5")], [], False)

        _check_merge(
            [("v1", "1"), ("v2", "2")],
            [("v1", "1"), ("v2", "2")],
            [("v1", "1"), ("v2", "2")],
            True,
        )

        _check_merge([("v1", "1"), ("v2", "2")], [("v1", "2"), ("v2", "1")], [], False)

        _check_merge([("v1", "1"), ("v2", "2")], [], [("v1", "1"), ("v2", "2")], True)

        _check_merge([], [("v1", "1"), ("v2", "2")], [("v1", "1"), ("v2", "2")], True)

        a1 = _build_assignment([("v1", "1")])
        a2 = _build_assignment([("v1", "1")])
        assert a1.freeze()
        with pytest.raises(AssertionError):
            a1.merge(a2)

    def test_compose(self):
        a1 = _build_assignment([("v1", "1")])
        a1.freeze()
        a2 = _build_assignment([("v2", "2")])
        a2.freeze()
        a3 = Assignment.compose([a1, a2])
        a4 = _build_assignment([("v1", "1"), ("v2", "2")])
        a4.freeze()
        assert a3 == a4

    def test_outplace_merge(self):
        a1 = _build_assignment([("v1", "1")])
        a2 = _build_assignment([("v2", "2")])
        a1.freeze()
        a2.freeze()
        a3 = a1.merge(a2, in_place=False)
        a4 = _build_assignment([("v1", "1"), ("v2", "2")])
        a4.freeze()
        assert a1 != a3
        assert a2 != a3
        assert a1 != a2
        assert a3 == a4
        assert not a1.__eq__(a2)
        assert not a1.__eq__(a3)
        assert not a2.__eq__(a3)
        assert a3.__eq__(a4)


class TestQueryAnswer:
    def _check_handle_set(self, atom, handles, count):
        assert len(handles) == len(count)
        query_answer = QueryAnswer(atom, None)
        assert query_answer.get_handle_set() == set(handles)
        handle_count = query_answer.get_handle_count()
        assert len(handle_count) == len(handles)
        for cursor in range(len(handles)):
            assert handle_count[handles[cursor]] == count[cursor]

    def test_get_handle_stats(self):
        self._check_handle_set(None, set([]), [])

        self._check_handle_set({"handle": "h1"}, ["h1"], [1])

        self._check_handle_set(
            {
                "handle": "h1",
                "targets": [
                    {"handle": "h2"},
                    {"handle": "h3"},
                ],
            },
            ["h1", "h2", "h3"],
            [1, 1, 1],
        )

        self._check_handle_set(
            {
                "handle": "h1",
                "targets": [
                    {"handle": "h2"},
                    {"handle": "h1"},
                ],
            },
            ["h1", "h2"],
            [2, 1],
        )

        self._check_handle_set(
            {
                "handle": "h1",
                "targets": [
                    {"handle": "h2"},
                    {
                        "handle": "h2",
                        "targets": [
                            {"handle": "h4"},
                            {"handle": "h1"},
                        ],
                    },
                    {
                        "handle": "h5",
                        "targets": [
                            {"handle": "h1"},
                            {"handle": "h6"},
                        ],
                    },
                    {"handle": "h3"},
                ],
            },
            ["h1", "h2", "h3", "h4", "h5", "h6"],
            [3, 2, 1, 1, 1, 1],
        )


@pytest.mark.parametrize(
    "version_string, expected",
    [
        ("1.2.3", (1, 2, 3)),
        ("10.20.30", (10, 20, 30)),
        ("0.0.0", (0, 0, 0)),
        ("invalid", None),
        ("1.2", None),
        ("1.2.3.4", None),
    ],
)
def test_get_version_components(version_string, expected):
    assert get_version_components(version_string) == expected


@pytest.mark.parametrize(
    "version1, version2, expected",
    [
        ("1.8.0", "1.8.1", True),
        ("1.8.0", "1.9.0", False),
        ("1.8.0", "2.8.0", False),
        ("1.8.0", "1.8.0", True),
        ("1.8", "1.8.0", None),
    ],
)
def test_compare_minor_versions(version1, version2, expected):
    assert compare_minor_versions(version1, version2) == expected


@pytest.mark.parametrize(
    "version1, version2, expected",
    [
        ("1.8.0", "1.8.1", False),
        ("1.8.0", "1.8.0", True),
        ("2.8.0", "1.8.0", False),
        ("1.8", "1.8.0", None),
    ],
)
def test_compare_patch_versions(version1, version2, expected):
    assert compare_patch_versions(version1, version2) == expected


@pytest.mark.parametrize(
    "version1, version2, expected",
    [
        ("1.8.0", "1.8.1", True),
        ("1.8.0", "2.8.0", False),
        ("1.8.0", "1.7.0", True),
        ("invalid", "1.8.0", None),
    ],
)
def test_compare_major_versions(version1, version2, expected):
    assert compare_major_versions(version1, version2) == expected
