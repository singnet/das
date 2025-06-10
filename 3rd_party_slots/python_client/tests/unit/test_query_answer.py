import pytest
from hyperon_das.query_answer import Assignment, QueryAnswer


class TestAssignment:
    def setup_method(self, method):
        self.assign = Assignment()
        self.assign.max_size = 2

    def test_assign_new_and_get(self):
        assert self.assign.assign("a", "1")
        assert self.assign.get("a") == "1"
        assert self.assign.variable_count() == 1

    def test_assign_existing_matching(self):
        self.assign.assign("a", "1")
        assert self.assign.assign("a", "1")
        assert self.assign.variable_count() == 1

    def test_assign_existing_mismatch(self):
        self.assign.assign("a", "1")
        assert not self.assign.assign("a", "2")
        assert self.assign.get("a") == "1"

    def test_assign_exceeds_max_size(self):
        self.assign.assign("a", "1")
        self.assign.assign("b", "2")
        with pytest.raises(ValueError) as exc:
            self.assign.assign("c", "3")
        assert "exceeds the maximal number" in str(exc.value)

    def test_is_compatible(self):
        a1 = Assignment()
        a2 = Assignment()
        a1.assign("x", "v1")
        a2.assign("x", "v1")
        a2.assign("y", "v2")
        assert a1.is_compatible(a2)
        a2.assign("x", "v3")
        assert a1.is_compatible(a2)

    def test_copy_from_and_add_assignments(self):
        a1 = Assignment()
        a2 = Assignment()
        a2.assign("x", "v1")
        a2.assign("y", "v2")
        a1.copy_from(a2)
        assert a1 == a2
        a3 = Assignment()
        a3.assign("x", "v1")
        a3.add_assignments(a2)
        assert a3.get("y") == "v2"

    def test_to_string_and_eq(self):
        self.assign.assign("a", "1")
        self.assign.assign("b", "2")
        s = self.assign.to_string()
        assert "(a: 1)" in s and "(b: 2)" in s
        other = Assignment()
        other.assign("a", "1")
        other.assign("b", "2")
        assert self.assign == other
        assert not (self.assign == object())


class TestQueryAnswer:
    def setup_method(self, method):
        self.q = QueryAnswer(handle="h1", importance=0.5)
        self.q.assignment.assign("a", "1")

    def test_default_and_add_handle(self):
        q2 = QueryAnswer()
        assert q2.handles == []
        q2.add_handle("h2")
        assert q2.handles == ["h2"]

    def test_copy(self):
        c = QueryAnswer.copy(self.q)
        assert c is not self.q
        assert c.handles == self.q.handles
        assert c.importance == self.q.importance
        assert c.assignment == self.q.assignment

    def test_merge_compatible_with_and_without_handles(self):
        other = QueryAnswer(handle="h2", importance=0.8)
        other.assignment.assign("b", "2")
        # merge handles
        result = self.q.merge(other, merge_handles=True)
        assert result
        assert set(self.q.handles) == {"h1", "h2"}
        assert self.q.importance == 0.8
        # merge without handles
        q3 = QueryAnswer.copy(self.q)
        q3.handles = ["h1"]
        q3.importance = 0.5
        result2 = q3.merge(other, merge_handles=False)
        assert result2
        assert q3.handles == ["h1"]
        assert q3.importance == 0.5

    def test_merge_incompatible(self):
        other = QueryAnswer(handle="h3")
        other.assignment.assign("a", "2")
        assert not self.q.merge(other)

    def test_tokenize_and_untokenize(self):
        self.q.handles = ["h1", "h2"]
        self.q.importance = 0.92345
        self.q.assignment = Assignment()
        self.q.assignment.assign("x", "v1")
        token_str = self.q.tokenize()
        new_q = QueryAnswer()
        new_q.untokenize(token_str)
        assert new_q.importance == 0.92345
        assert new_q.handles == ["h1", "h2"]
        assert new_q.assignment.get("x") == "v1"

    @pytest.mark.parametrize("token_str", [
        "",  # empty
        "badfloat 1 h1 0",  # invalid importance
        "1.0 badint h1 0",  # invalid handles size
        "1.0 -1",  # negative handles size
        "1.0 1 h1 bad",  # invalid assignment size
        "1.0 1 h1 -1",  # negative assignment size
        "1.0 0 1 extra",  # extra token
    ])
    def test_untokenize_errors(self, token_str):
        with pytest.raises(ValueError):
            QueryAnswer().untokenize(token_str)

    def test_to_string_format(self):
        s = self.q.to_string()
        assert s == "QueryAnswer<1,1> [h1] {(a: 1)} 0.5"
        assert self.q.assignment.to_string() in s
