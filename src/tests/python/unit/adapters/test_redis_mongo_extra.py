from unittest import mock

import pytest

from hyperon_das_atomdb.adapters.redis_mongo_db import MongoDBIndex, RedisMongoDB, _HashableDocument
from tests.python.helpers import dict_to_node_params
from tests.python.unit.fixtures import redis_mongo_db  # noqa: F401


class TestRedisMongoExtra:
    def test_hashable_document_str(self, redis_mongo_db):  # noqa: F811
        db = redis_mongo_db
        node = db._build_node(dict_to_node_params({"type": "A", "name": "A"}))
        hashable = _HashableDocument(node)
        str_hashable = str(hashable)
        assert isinstance(str_hashable, str)
        assert hashable
        assert str(node) == str_hashable

    @pytest.mark.parametrize(
        "params",
        [
            {"atom_type": "A", "fields": []},
            {"atom_type": "A", "fields": None},
        ],
    )
    def test_index_create_exceptions(self, params, request):
        db = request.getfixturevalue("redis_mongo_db")
        mi = MongoDBIndex(db.mongo_db)
        with pytest.raises(ValueError):
            mi.create(**params)

    @mock.patch(
        "hyperon_das_atomdb.adapters.redis_mongo_db.MongoClient", return_value=mock.MagicMock()
    )
    @mock.patch("hyperon_das_atomdb.adapters.redis_mongo_db.Redis", return_value=mock.MagicMock())
    @mock.patch(
        "hyperon_das_atomdb.adapters.redis_mongo_db.RedisCluster", return_value=mock.MagicMock()
    )
    def test_create_db_connection_mongo(self, mock_mongo, mock_redis, mock_redis_cluster):
        RedisMongoDB(mongo_tls_ca_file="/tmp/mock", redis_password="12", redis_username="A")
        RedisMongoDB(redis_cluster=False)
