import os
import subprocess
from enum import Enum
from typing import List

redis_port = "29000"
mongo_port = "28000"
scripts_path = "./tests/integration/scripts"
devnull = open(os.devnull, "w")

DAS_MONGODB_HOSTNAME = os.environ.get("DAS_MONGODB_HOSTNAME")
DAS_MONGODB_PORT = os.environ.get("DAS_MONGODB_PORT")
DAS_MONGODB_USERNAME = os.environ.get("DAS_MONGODB_USERNAME")
DAS_MONGODB_PASSWORD = os.environ.get("DAS_MONGODB_PASSWORD")
DAS_REDIS_HOSTNAME = os.environ.get("DAS_REDIS_HOSTNAME")
DAS_REDIS_PORT = os.environ.get("DAS_REDIS_PORT")
DAS_REDIS_USERNAME = os.environ.get("DAS_REDIS_USERNAME")
DAS_REDIS_PASSWORD = os.environ.get("DAS_REDIS_PASSWORD")
DAS_USE_REDIS_CLUSTER = os.environ.get("DAS_USE_REDIS_CLUSTER")
DAS_USE_REDIS_SSL = os.environ.get("DAS_USE_REDIS_SSL")

os.environ["DAS_MONGODB_HOSTNAME"] = "localhost"
os.environ["DAS_MONGODB_PORT"] = mongo_port
os.environ["DAS_MONGODB_USERNAME"] = "dbadmin"
os.environ["DAS_MONGODB_PASSWORD"] = "dassecret"
os.environ["DAS_REDIS_HOSTNAME"] = "localhost"
os.environ["DAS_REDIS_PORT"] = redis_port
os.environ["DAS_REDIS_USERNAME"] = ""
os.environ["DAS_REDIS_PASSWORD"] = ""
os.environ["DAS_USE_REDIS_CLUSTER"] = "false"
os.environ["DAS_USE_REDIS_SSL"] = "false"


class Database(Enum):
    REDIS = "redis"
    MONGO = "mongo"


class PyMongoFindExplain:
    def __init__(self, collection):
        self.collection = collection
        self.original_find_function = collection.find

    def __enter__(self):
        # Adds explain to internal pymongo's find
        self.explain = []

        def find_explain(*args, **kwargs):
            find = self.original_find_function(*args, **kwargs)
            self.explain.append(find.explain())
            return find

        self.collection.find = find_explain
        return self.explain

    def __exit__(self, exc_type, exc_value, traceback):
        self.collection.find = self.original_find_function


def _db_up(*database_names: List[Database]):
    if database_names:
        for database_name in database_names:
            subprocess.call(
                [
                    "bash",
                    f"{scripts_path}/{database_name.value}-up.sh",
                    eval(f"{database_name.value}_port"),
                ],
                stdout=devnull,
                stderr=devnull,
            )
    else:
        subprocess.call(
            ["bash", f"{scripts_path}/redis-up.sh", redis_port],
            stdout=devnull,
            stderr=devnull,
        )
        subprocess.call(
            ["bash", f"{scripts_path}/mongo-up.sh", mongo_port],
            stdout=devnull,
            stderr=devnull,
        )


def _db_down():
    subprocess.call(
        ["bash", f"{scripts_path}/redis-down.sh", redis_port],
        stdout=devnull,
        stderr=devnull,
    )
    subprocess.call(
        ["bash", f"{scripts_path}/mongo-down.sh", mongo_port],
        stdout=devnull,
        stderr=devnull,
    )


def cleanup(request):
    def restore_environment():
        if DAS_MONGODB_HOSTNAME:
            os.environ["DAS_MONGODB_HOSTNAME"] = DAS_MONGODB_HOSTNAME
        if DAS_MONGODB_PORT:
            os.environ["DAS_MONGODB_PORT"] = DAS_MONGODB_PORT
        if DAS_MONGODB_USERNAME:
            os.environ["DAS_MONGODB_USERNAME"] = DAS_MONGODB_USERNAME
        if DAS_MONGODB_PASSWORD:
            os.environ["DAS_MONGODB_PASSWORD"] = DAS_MONGODB_PASSWORD
        if DAS_REDIS_HOSTNAME:
            os.environ["DAS_REDIS_HOSTNAME"] = DAS_REDIS_HOSTNAME
        if DAS_REDIS_PORT:
            os.environ["DAS_REDIS_PORT"] = DAS_REDIS_PORT
        if DAS_REDIS_USERNAME:
            os.environ["DAS_REDIS_USERNAME"] = DAS_REDIS_USERNAME
        if DAS_REDIS_PASSWORD:
            os.environ["DAS_REDIS_PASSWORD"] = DAS_REDIS_PASSWORD
        if DAS_USE_REDIS_CLUSTER:
            os.environ["DAS_USE_REDIS_CLUSTER"] = DAS_USE_REDIS_CLUSTER
        if DAS_USE_REDIS_SSL:
            os.environ["DAS_USE_REDIS_SSL"] = DAS_USE_REDIS_SSL

    def enforce_containers_removal():
        _db_down()

    request.addfinalizer(restore_environment)
    request.addfinalizer(enforce_containers_removal)
