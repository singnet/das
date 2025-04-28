from unittest import mock

import mongomock
import pytest

from hyperon_das_atomdb.adapters.ram_only import InMemoryDB
from hyperon_das_atomdb.adapters.redis_mongo_db import RedisMongoDB


class MockRedis:
    def __init__(self):
        self.cache = dict()

    def get(self, key):
        if key in self.cache:
            return self.cache[key]
        return None

    def set(self, key, value, *args, **kwargs):
        if isinstance(self.cache, dict):
            self.cache[key] = value
            return "OK"
        return None

    def hget(self, hash, key):
        if hash in self.cache:
            if key in self.cache[hash]:
                return self.cache[hash][key]
        return None

    def hset(self, hash, key, value, *args, **kwargs):
        if isinstance(self.cache, dict):
            self.cache[hash][key] = value
            return 1
        return None

    def exists(self, key):
        if key in self.cache:
            return 1
        return 0

    def cache_overwrite(self, cache):
        self.cache = cache or dict()

    def sadd(self, key, *members):
        if key not in self.cache:
            self.cache[key] = set()
        before_count = len(self.cache[key])
        self.cache[key].update(members)
        after_count = len(self.cache[key])
        return after_count - before_count

    def smembers(self, key):
        if key in self.cache:
            return self.cache[key]
        return set()

    def flushall(self):
        self.cache.clear()

    def delete(self, *keys):
        deleted_count = 0
        for key in keys:
            if key in self.cache:
                del self.cache[key]
                deleted_count += 1
        return deleted_count

    def getdel(self, key):
        value = self.cache.get(key)
        if key in self.cache:
            del self.cache[key]
        return value

    def srem(self, key, *members):
        if key not in self.cache:
            return 0
        initial_count = len(self.cache[key])
        self.cache[key].difference_update(members)
        removed_count = initial_count - len(self.cache[key])
        return removed_count

    def sscan(self, name, cursor=0, match=None, count=None):
        key = name
        if key not in self.cache:
            return (0, [])

        elements = list(self.cache[key])
        if match:
            elements = [e for e in elements if match in e]
        start = cursor
        end = min(start + (count if count else len(elements)), len(elements))
        new_cursor = end if end < len(elements) else 0

        return (new_cursor, elements[start:end])
    
    def zrange(self, name: str, start: int, end: int, withscores: bool = False):
        if name not in self.cache:
            return []

        sorted_items = sorted(self.cache[name].items(), key=lambda item: (item[1], item[0]))
        length = len(sorted_items)

        if start < 0:
            start = max(0, length + start)
        if end < 0:
            end = length + end

        sliced = sorted_items[start:end + 1]
        if withscores:
            return sliced
        else:
            return [member for member, score in sliced]

    def zadd(self, name: str, mapping: dict):
        if name not in self.cache:
            self.cache[name] = {}
        added_count = 0
        for member, score in mapping.items():
            if member not in self.cache[name]:
                added_count += 1
            self.cache[name][member] = score
        return added_count

    def zrem(self, name: str, *values):
        if name not in self.cache or not isinstance(self.cache[name], dict):
            return 0
        removed_count = 0
        for value in values:
            if value in self.cache[name]:
                del self.cache[name][value]
                removed_count += 1
        return removed_count

def mongo_mock():
    return mongomock.MongoClient().db


def redis_mock():
    return MockRedis()


@pytest.fixture
def redis_mongo_db():
    mongo_db = mongo_mock()
    redis_db = redis_mock()
    with (
        mock.patch(
            "hyperon_das_atomdb.adapters.redis_mongo_db.RedisMongoDB._connection_mongo_db",
            return_value=mongo_db,
        ),
        mock.patch(
            "hyperon_das_atomdb.adapters.redis_mongo_db.RedisMongoDB._connection_redis",
            return_value=redis_db,
        ),
    ):
        db = RedisMongoDB()

        yield db


@pytest.fixture
def in_memory_db():
    db = InMemoryDB()
    yield db
