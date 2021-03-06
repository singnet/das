import os
import re

from typing import List
import pytest
from couchbase.auth import PasswordAuthenticator
from couchbase.bucket import Bucket
from couchbase.cluster import Cluster

from das.helpers import get_mongodb
from das.pattern_matcher.db_interface import DBInterface
from das.pattern_matcher.couch_mongo_db import CouchMongoDB
from das.couchbase_schema import CollectionNames as CouchbaseCollectionNames
from das.mongo_schema import CollectionNames as MongoCollectionNames, FieldNames as MongoFieldNames

@pytest.fixture()
def mongo_db():
    mongodb_specs = {
        "hostname": "mongo",
        "port": 27017,
        "username": "dbadmin",
        "password": "dassecret",
        "database": "TOY",
    }
    return get_mongodb(mongodb_specs)


@pytest.fixture()
def couch_db():
    couchbase_specs = {
        "hostname": "couchbase",
        "username": "dbadmin",
        "password": "dassecret",
    }
    cluster = Cluster(
        f'couchbase://{couchbase_specs["hostname"]}',
        authenticator=PasswordAuthenticator(
            couchbase_specs["username"], couchbase_specs["password"]
        ),
    )
    return cluster.bucket("das")


@pytest.fixture()
def db(couch_db, mongo_db):
    return CouchMongoDB(couch_db, mongo_db)

NODE_SPECS = [('Concept', 'human'),
              ('Concept', 'monkey'),
              ('Concept', 'chimp'),
              ('Concept', 'snake'),
              ('Concept', 'earthworm'),
              ('Concept', 'rhino'),
              ('Concept', 'triceratops'),
              ('Concept', 'vine'),
              ('Concept', 'ent'),
              ('Concept', 'mammal'),
              ('Concept', 'animal'),
              ('Concept', 'reptile'),
              ('Concept', 'dinosaur'),
              ('Concept', 'plant')]

def _add_node_names(db, txt):
    handles = re.findall("'[a-z0-9]{32}'", txt)
    for quoted_handle in handles:
        handle = quoted_handle[1:-1]
        try:
            node_name = db.get_node_name(handle)
            txt = re.sub(quoted_handle, f'{quoted_handle} ({node_name})', txt, count=1)
        except Exception:
            pass
    return txt

def test_db_creation(db: DBInterface):
    assert db.couch_db
    assert db.mongo_db
    assert db.couch_incoming_collection
    assert db.couch_outgoing_collection
    assert db.couch_patterns_collection
    assert len(db.node_documents) == 14
    assert len(db.node_handles) == 14
    assert len(db.atom_type_hash) == 5
    assert len(db.atom_type_hash_reverse) == 5
    assert len(db.type_hash) == 5

def test_node_exists(db: DBInterface):
    assert db.node_exists('Concept', 'human')
    assert db.node_exists('Concept', 'monkey')
    assert db.node_exists('Concept', 'chimp')
    assert db.node_exists('Concept', 'snake')
    assert db.node_exists('Concept', 'earthworm')
    assert db.node_exists('Concept', 'rhino')
    assert db.node_exists('Concept', 'triceratops')
    assert db.node_exists('Concept', 'vine')
    assert db.node_exists('Concept', 'ent')
    assert db.node_exists('Concept', 'mammal')
    assert db.node_exists('Concept', 'animal')
    assert db.node_exists('Concept', 'reptile')
    assert db.node_exists('Concept', 'dinosaur')
    assert db.node_exists('Concept', 'plant')
    assert not db.node_exists('blah', 'plant')
    assert not db.node_exists('Concept', 'blah')

def _check_link(db: DBInterface, handle: str, link_type: str, target1: str, target2: str):
    collection = db.mongo_db.get_collection(MongoCollectionNames.LINKS_ARITY_2)
    document = collection.find_one({'_id': handle})
    type_handle = db.atom_type_hash[link_type]
    assert document
    assert document['key1'] == type_handle
    assert document['key2'] == target1
    assert document['key3'] == target2

def _get_mongo_document(db: DBInterface, handle: str):
    collection = db.mongo_db.get_collection(MongoCollectionNames.LINKS_ARITY_2)
    document = collection.find_one({'_id': handle})
    return document
    
def test_get_link_handle(db: DBInterface):
    human = db.get_node_handle('Concept', 'human')
    monkey = db.get_node_handle('Concept', 'monkey')
    mammal = db.get_node_handle('Concept', 'mammal')
    handle = db.get_link_handle('Inheritance', [human, mammal])
    _check_link(db, handle, 'Inheritance', human, mammal)
    handle = db.get_link_handle('Inheritance', [monkey, mammal])
    _check_link(db, handle, 'Inheritance', monkey, mammal)
    with pytest.raises(ValueError):
        db.get_link_handle('Inheritance', [monkey, human])
    handle = db.get_link_handle('Similarity', [human, monkey])
    _check_link(db, handle, 'Similarity', human, monkey)
    handle = db.get_link_handle('Similarity', [monkey, human])
    _check_link(db, handle, 'Similarity', human, monkey)
    with pytest.raises(ValueError):
        db.get_link_handle('Similarity', [mammal, human])
    with pytest.raises(ValueError):
        db.get_link_handle('Similarity', [human, mammal])

def test_link_exists(db: DBInterface):
    human = db.get_node_handle('Concept', 'human')
    monkey = db.get_node_handle('Concept', 'monkey')
    mammal = db.get_node_handle('Concept', 'mammal')
    assert db.link_exists('Inheritance', [human, mammal])
    assert db.link_exists('Inheritance', [monkey, mammal])
    assert not db.link_exists('Inheritance', [monkey, human])
    assert db.link_exists('Similarity', [human, monkey])
    assert db.link_exists('Similarity', [monkey, human])
    assert not db.link_exists('Similarity', [mammal, human])
    assert not db.link_exists('Similarity', [human, mammal])

def test_get_node_handle(db: DBInterface):
    node_names = ['human', 'monkey', 'chimp', 'snake', 'earthworm', 'rhino', 'triceratops',
                  'vine', 'ent', 'mammal', 'animal', 'reptile', 'dinosaur', 'plant']
    for name in node_names:
        handle = db.get_node_handle('Concept', name)
        collection = db.mongo_db.get_collection(MongoCollectionNames.NODES)
        document = collection.find_one({'_id': handle})
        assert document[MongoFieldNames.NODE_NAME] == name
    with pytest.raises(ValueError):
        db.get_node_handle('Concept', 'blah')

def _check_link_targets(db: DBInterface, handle: str, target_handles: List[str], ordered: bool):
    assert len(target_handles) == 2
    document = _get_mongo_document(db, handle)
    if ordered:
        assert document['key2'] == target_handles[0]
        assert document['key3'] == target_handles[1]
    else:
        assert document['key2'] == target_handles[0] or document['key2'] == target_handles[1]
        assert document['key3'] == target_handles[0] or document['key3'] == target_handles[1]

def test_get_link_targets(db: DBInterface):
    human = db.get_node_handle('Concept', 'human')
    monkey = db.get_node_handle('Concept', 'monkey')
    mammal = db.get_node_handle('Concept', 'mammal')
    handle = db.get_link_handle('Inheritance', [human, mammal])
    _check_link_targets(db, handle, [human, mammal], True)
    with pytest.raises(AssertionError):
        _check_link_targets(db, handle, [mammal, human], True)
    handle = db.get_link_handle('Inheritance', [monkey, mammal])
    _check_link_targets(db, handle, [monkey, mammal], True)
    with pytest.raises(AssertionError):
        _check_link_targets(db, handle, [mammal, monkey], True)
    with pytest.raises(AssertionError):
        _check_link_targets(db, handle, [monkey, monkey], True)
    handle = db.get_link_handle('Similarity', [human, monkey])
    _check_link_targets(db, handle, [human, monkey], False)
    _check_link_targets(db, handle, [monkey, human], False)
    handle = db.get_link_handle('Similarity', [monkey, human])
    _check_link_targets(db, handle, [human, monkey], False)
    _check_link_targets(db, handle, [monkey, human], False)
    with pytest.raises(AssertionError):
        _check_link_targets(db, handle, [monkey, mammal], False)
    with pytest.raises(AssertionError):
        _check_link_targets(db, handle, [mammal, monkey], False)

def test_is_ordered(db: DBInterface):
    human = db.get_node_handle('Concept', 'human')
    monkey = db.get_node_handle('Concept', 'monkey')
    mammal = db.get_node_handle('Concept', 'mammal')
    assert db.is_ordered(db.get_link_handle('Inheritance', [human, mammal]))
    assert not db.is_ordered(db.get_link_handle('Similarity', [human, monkey]))
    with pytest.raises(ValueError):
        db.is_ordered(db.get_link_handle('Inheritance', [human, monkey]))

def test_get_all_nodes(db: DBInterface):
    nodes_in_db = db.get_all_nodes('Concept')
    assert len(nodes_in_db) == 14
    for node_type, node_name in NODE_SPECS:
        node = db.get_node_handle(node_type, node_name)
        assert node in nodes_in_db
    with pytest.raises(ValueError):
        nodes_in_db = db.get_all_nodes('blah')
    
def test_get_matched_links(db: DBInterface):
    # TODO: once we have API to add nodes/links, add a
    #       testcase like Eval(PN, List(X, Y)) where the
    #       pattern is one level below the logic expression
    #       used to call matched()
    mammal = db.get_node_handle('Concept', 'mammal')
    animal = db.get_node_handle('Concept', 'animal')
    human = db.get_node_handle('Concept', 'human')
    monkey = db.get_node_handle('Concept', 'monkey')
    chimp = db.get_node_handle('Concept', 'chimp')
    assert len(db.get_matched_links('Inheritance', ['*', '*'])) == 12
    assert len(db.get_matched_links('Inheritance', ['*', mammal])) == 4
    assert len(db.get_matched_links('Inheritance', [mammal, '*'])) == 1
    assert len(db.get_matched_links('Inheritance', ['*', animal])) == 3
    assert len(db.get_matched_links('Inheritance', [animal, '*'])) == 0
    assert len(db.get_matched_links('Inheritance', [mammal, animal])) == 1
    assert len(db.get_matched_links('Inheritance', [chimp, mammal])) == 1
    assert len(db.get_matched_links('Inheritance', [animal, mammal])) == 0
    assert len(db.get_matched_links('Similarity', ['*', '*'])) == 7
    assert len(db.get_matched_links('Similarity', [human, '*'])) == 3
    assert len(db.get_matched_links('Similarity', ['*', human])) == 3
    assert len(db.get_matched_links('Similarity', [monkey, '*'])) == 2
    assert len(db.get_matched_links('Similarity', ['*', monkey])) == 2
    assert len(db.get_matched_links('Similarity', [chimp, '*'])) == 2
    assert len(db.get_matched_links('Similarity', ['*', chimp])) == 2
    assert len(db.get_matched_links('Similarity', [monkey, human])) == 1
    assert len(db.get_matched_links('Similarity', [human, monkey])) == 1
    assert len(db.get_matched_links('Similarity', [human, mammal])) == 0
    assert len(db.get_matched_links('Similarity', [mammal, human])) == 0

def test_build_hash_template(db: DBInterface):
    v1 = db._build_hash_template(['Inheritance', 'Concept', 'Concept'])
    v2 = db._build_hash_template(['Similarity', 'Concept', 'Concept'])
    v3 = db._build_hash_template(['Similarity', 'Concept', ['Inheritance', 'Concept', 'Concept']])
    assert len(v1) == 3
    assert len(v2) == 3
    assert len(v3) == 3
    assert len(v3[2]) == 3
    assert v1[1] == v1[2] and v1[0] != v1[1]
    assert v2[1] == v2[2] and v2[0] != v2[1]
    assert v1[0] != v2[0] and v1[1] == v2[1]
    assert v3[0] == v2[0]
    assert v3[1] == v1[1]
    assert v3[2][0] == v1[0]
    assert v3[2][1] == v1[1]
    assert v3[2][2] == v1[2]

def test_get_matched_type_template(db: DBInterface):
    v1 = db.get_matched_type_template(['Inheritance', 'Concept', 'Concept'])
    v2 = db.get_matched_type_template(['Similarity', 'Concept', 'Concept'])
    with pytest.raises(ValueError):
        v3 = db.get_matched_type_template(['Inheritance', 'Concept', 'blah'])
    with pytest.raises(ValueError):
        v4 = db.get_matched_type_template(['Similarity', 'blah', 'Concept'])
    assert(len(v1) == 12)
    assert(len(v2) == 7)
    v5 = db.get_matched_links('Inheritance', ['*', '*'])
    v6 = db.get_matched_links('Similarity', ['*', '*'])
    assert(v1 == v5)
    assert(v2 == v6)

def test_get_node_name(db: DBInterface):
    for node_type, node_name in NODE_SPECS:
        handle = db.get_node_handle(node_type, node_name)
        db_name = db.get_node_name(handle)
        assert db_name == node_name

def test_get_matched_node_name(db: DBInterface):
    assert sorted(db.get_matched_node_name('Concept', 'ma')) == sorted([
        db.get_node_handle('Concept', 'human'),
        db.get_node_handle('Concept', 'mammal'),
        db.get_node_handle('Concept', 'animal'),])
    assert sorted(db.get_matched_node_name('blah', 'Concept')) == []
    assert sorted(db.get_matched_node_name('Concept', 'blah')) == []
