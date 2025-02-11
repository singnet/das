from enum import Enum, auto


class QueryOutputFormat(int, Enum):
    HANDLE = auto()
    ATOM_INFO = auto()
    JSON = auto()


class DasType(str, Enum):
    LOCAL_RAM_ONLY = "local_ram_only"
    LOCAL_REDIS_MONGO = "local_redis_mongo"
    REMOTE = "remote"
