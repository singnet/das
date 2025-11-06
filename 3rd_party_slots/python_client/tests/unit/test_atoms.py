import pytest

from hyperon_das.commons.atoms import Atom, Link, Node
from hyperon_das.commons.atoms.handle_decoder import HandleDecoder
from hyperon_das.commons.properties import Properties
from hyperon_das.hasher import Hasher, composite_hash
from hyperon_das.logger import log
from hyperon_das.query_answer import Assignment


class HandleDecoderMock(HandleDecoder):
    def get_atom(self, handle) -> Atom:
        if 'node' in handle:
            return Node(type='Symbol', name=handle)
        elif 'link' in handle:
            return Link(type='Expression', targets=['node1', 'node2', 'node3'])
        else:
            return None


class TestNode:
    def test_init_success_and_fields(self):
        p = Properties()
        p['is_literal'] = "true"
        n = Node(type='Symbol', name='"Alice"', is_toplevel=True, custom_attributes=p)
        assert n.type == 'Symbol'
        assert n.name == '"Alice"'
        assert n.is_toplevel is True
        assert n.custom_attributes == p

    def test_init_missing_type_or_name_raises_valueerror(self):
        with pytest.raises(ValueError):
            Node(type=None, name=None)

        with pytest.raises(ValueError):
            Node(type='X', name=None)

    def test_init_with_tokens_and_roundtrip_tokenize_untokenize(self):
        orig = Node(type='Symbol', name='bob', is_toplevel=False)
        tokens = []
        orig.tokenize(tokens)

        assert tokens, ['Symbol', 'false', '0', 'bob']

        tcopy = tokens.copy()
        new = Node(tokens=tcopy)
        assert new == orig

    def test_init_with_other_makes_copy(self):
        prop = Properties()
        prop['x'] = 'y'
        a = Node(type='Symbol', name='bob', is_toplevel=False, custom_attributes=prop)
        b = Node(other=a)
        assert b.type == a.type
        assert b.name == a.name
        assert b.custom_attributes == a.custom_attributes
        assert b is not a

    def test_validate(self):
        n = Node(type='Symbol', name='z')
        n.type = Atom.UNDEFINED_TYPE

        with pytest.raises(RuntimeError):
            n.validate()

        n.name = ''
        with pytest.raises(RuntimeError):
            n.validate()

    def test_eq_and_ne_behavior(self):
        p1 = Properties()
        p1['k'] = 'v'
        n1 = Node(type='T', name='n', custom_attributes=p1)
        n2 = Node(type='T', name='n', custom_attributes=p1)
        assert n1 == n2
        n2.name = 'different'
        assert n1 != n2
        assert (n1 == object()) is False

    def test_copy_from_returns_self_and_copies_all_fields(self):
        p1 = Properties()
        p1['a'] = 'b'
        p2 = Properties()
        node1 = Node(type='X', name='node1', custom_attributes=p1, is_toplevel=True)
        node2 = Node(type='Y', name='node2', custom_attributes=p2)
        returned = node2.copy_from(node1)
        assert returned is node2
        assert node2.name == 'node1'
        assert node2.type == 'X'
        assert node2.is_toplevel is True
        assert node2.custom_attributes == node1.custom_attributes

    def test_tokenize_with_properties_nonzero_and_untokenize_handles_props(self):
        attr = Properties()
        attr['k1'] = 'v1'
        attr['k2'] = 'v2'

        n = Node(type='T', name='N', custom_attributes=attr)
        tokens = []
        n.tokenize(tokens)

        assert tokens[0] == 'T'

        tcopy = tokens.copy()
        new = Node(tokens=tcopy)

        assert new == n
        assert new.custom_attributes == attr

    def test_handle_and_match_true_and_false(self):
        n = Node(type='T', name='N')
        expected_handle = Hasher.node_handle('T', 'N')
        assert n.handle() == expected_handle
        assert n.match(expected_handle, Assignment(), HandleDecoderMock())
        assert not n.match('some:other:handle', Assignment(), HandleDecoderMock())

    def test_metta_representation_symbol_does_not_log_but_non_symbol_logs(self, monkeypatch):
        errors = []
        monkeypatch.setattr(log, "error", lambda msg: errors.append(msg))

        n_sym = Node(type='Symbol', name='"S"')
        assert n_sym.metta_representation(HandleDecoderMock()) == '"S"'

        n_other = Node(type='NotSymbol', name='"N"')

        with pytest.raises(RuntimeError):
            n_other.metta_representation(HandleDecoderMock())

        assert any("Can't compute metta expression of node whose type (NotSymbol) is not Symbol" in m for m in errors)

    def test_to_string_contains_fields(self):
        attr = Properties()
        attr['is_literal'] = True
        n = Node(type='ZZ', name='nm', custom_attributes=attr)
        s = n.to_string()
        assert "ZZ" in s
        assert "nm" in s
        assert attr.to_string() in s


class TestLink:
    def test_init_success_and_fields(self):
        attr = Properties()
        attr['k'] = 'v'
        link = Link(type='Expression', targets=['t1', 't2'], is_toplevel=True, custom_attributes=attr)
        assert link.type == 'Expression'
        assert link.targets == ['t1', 't2']
        assert link.is_toplevel is True
        assert link.custom_attributes == attr

    def test_init_missing_type_or_targets_raises_valueerror(self):
        with pytest.raises(ValueError):
            Link(type=None, targets=None)
        with pytest.raises(ValueError):
            Link(type='X', targets=None)

    def test_init_with_tokens_roundtrip_tokenize_untokenize(self):
        attr = Properties()
        attr['k'] = 'v'
        original = Link(type='Expression', targets=['t1', 't2'], is_toplevel=False, custom_attributes=attr)
        tokens = []
        original.tokenize(tokens)
        tokens_copy = tokens.copy()
        reconstructed = Link(tokens=tokens_copy)
        assert reconstructed.targets == original.targets
        assert reconstructed.type == original.type
        assert reconstructed.is_toplevel == original.is_toplevel
        assert reconstructed.custom_attributes == original.custom_attributes

    def test_init_with_other_makes_copy(self):
        attr = Properties()
        attr['k'] = 'v'
        source = Link(type='Expression', targets=['x'], custom_attributes=attr)
        copy_link = Link(other=source)

        assert copy_link.type == source.type
        assert copy_link.targets == source.targets
        assert copy_link.custom_attributes == source.custom_attributes
        assert copy_link is not source

    def test_validate_undefined_type_errors_and_raises(self, monkeypatch):
        errors = []
        monkeypatch.setattr(log, "error", lambda msg: errors.append(msg))

        link = Link(type='Expression', targets=['a'])
        link.type = Atom.UNDEFINED_TYPE

        with pytest.raises(Exception):
            link.validate()
        assert any("Link type can't be" in m for m in errors)

        link = Link(type='Expression', targets=['a'])
        link.targets = []

        with pytest.raises(Exception):
            link.validate()
        assert any("Link must have at least 1 target" in m for m in errors)

    def test_eq_and_ne_behavior(self):
        attr = Properties()
        attr['k'] = 'v'
        l1 = Link(type='T', targets=['n1', 'n2'], custom_attributes=attr)

        attr2 = Properties()
        attr2['k'] = 'v'
        l2 = Link(type='T', targets=['n1', 'n2'], custom_attributes=attr2)
        assert l1 == l2

        l2.targets = ['other']
        assert l1 != l2
        assert (l1 == object()) is False

    def test_copy_from_returns_self_and_copies_all_fields(self):
        attr = Properties()
        attr['a'] = 'b'
        link1 = Link(type='X', targets=['t1', 't2'], custom_attributes=attr, is_toplevel=True)
        link2 = Link(type='Y', targets=['c'])
        returned = link2.copy_from(link1)

        assert returned is link2
        assert link2.targets == ['t1', 't2']
        assert link2.type == 'X'
        assert link2.is_toplevel is True
        assert link2.custom_attributes == link1.custom_attributes

    def test_to_string_contains_fields(self):
        attr = Properties()
        attr['x'] = 'y'
        link = Link(type='L', targets=['t1', 't2'], custom_attributes=attr, is_toplevel=False)
        s = link.to_string()
        assert "L" in s
        assert "t1" in s and "t2" in s
        assert attr.to_string() in s

    def test_arity_and_tokenize_untokenize_props(self):
        attr = Properties()
        attr['k'] = 'v'
        link = Link(type='Expression', targets=['t1', 't2', 't3'], custom_attributes=attr)
        assert link.arity() == 3
        tokens = []
        link.tokenize(tokens)
        tcopy = tokens.copy()
        new = Link(tokens=tcopy)
        assert new.targets == link.targets
        assert new.custom_attributes == attr

    def test_composite_type_success_and_failure(self, monkeypatch):
        link = Link(type='T', targets=['node1', 'node2'])
        comp = link.composite_type(HandleDecoderMock())

        assert comp[0] == Hasher.type_handle('T')
        assert comp[1] == Hasher.type_handle('Symbol')
        assert comp[2] == Hasher.type_handle('Symbol')

        errors = []
        monkeypatch.setattr(log, "error", lambda msg: errors.append(msg))

        link2 = Link(type='T', targets=['node1', 'missing'])

        with pytest.raises(RuntimeError):
            link2.composite_type(HandleDecoderMock())

        assert any('Unkown atom with handle' in m for m in errors)

    def test_composite_type_hash_uses_composite_hash(self):
        link = Link(type='Expression', targets=['node1'])
        ch = link.composite_type_hash(HandleDecoderMock())
        elements = [Hasher.type_handle('Expression'), Hasher.type_handle('Symbol')]
        assert composite_hash(elements, 2) in ch

    def test_handle_and_match_true_and_false(self):
        link = Link(type='MyType', targets=['t'])
        expected = Hasher.link_handle('MyType', ['t'])
        assert link.handle() == expected
        assert link.match(expected, Assignment(), HandleDecoderMock())
        assert not link.match('other', Assignment(), HandleDecoderMock())

    def test_metta_representation_success_and_failure(self, monkeypatch):
        errors = []
        monkeypatch.setattr(log, "error", lambda msg: errors.append(msg))

        link = Link(type='Expression', targets=['node1', 'node2'])
        m = link.metta_representation(HandleDecoderMock())
        assert m == '(node1 node2)'

        l2 = Link(type='Expression', targets=['link1', 'nodeA', 'nodeB'])
        m2 = l2.metta_representation(HandleDecoderMock())
        assert m2 == '((node1 node2 node3) nodeA nodeB)'

        l3 = Link(type='Fake', targets=['missing'])

        with pytest.raises(RuntimeError):
            l3.metta_representation(HandleDecoderMock())

        assert any("Can't compute metta expression of link whose type (Fake) is not Expression" in e for e in errors)
