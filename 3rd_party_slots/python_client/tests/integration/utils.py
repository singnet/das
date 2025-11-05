from pymongo.mongo_client import MongoClient

from hyperon_das.commons.atoms import Atom, Link, Node
from hyperon_das.commons.atoms.handle_decoder import HandleDecoder
from hyperon_das.commons.properties import Properties
from hyperon_das.logger import log

DAS_MONGODB_HOSTNAME = "0.0.0.0"
DAS_MONGODB_PORT = 28000
DAS_MONGODB_USERNAME = "dbadmin"
DAS_MONGODB_PASSWORD = "dassecret"


class AtomDecoder(HandleDecoder):
    def __init__(self, **kwargs):
        self.setup_mongo_db(**kwargs)
        self.nodes_collection = 'nodes'
        self.links_collection = 'links'

    def setup_mongo_db(
        self, hostname: str = None, port: int = None, username: str = None, password: str = None
    ) -> None:
        hostname = hostname or DAS_MONGODB_HOSTNAME
        port = port or DAS_MONGODB_PORT
        username = username or DAS_MONGODB_USERNAME
        password = password or DAS_MONGODB_PASSWORD

        mongo_url = f"mongodb://{username}:{password}@{hostname}:{port}"
        try:
            self.mongo_db = MongoClient(mongo_url)['das']
        except ValueError as e:
            log.error(f"An error occurred while creating a MongoDB client - Details: {str(e)}")
            raise

    def get_atom(self, handle: str) -> Atom | None:
        try:
            node_collection = self.mongo_db[self.nodes_collection]

            try:
                doc = node_collection.find_one({"_id": handle})
                if doc:
                    return self.build_node(doc)
            except Exception as e:
                log.error(str(e))
                raise

            link_collection = self.mongo_db[self.links_collection]

            try:
                doc = link_collection.find_one({"_id": handle})
                if doc:
                    return self.build_link(doc)
            except Exception as e:
                log.error(str(e))
                raise

            return None

        except Exception as e:
            log.error(f"Failed to retrieve atom for handle={handle} - Details: {e}")
            return None

    def build_node(self, doc) -> Node:
        return Node(
            type=doc['named_type'], name=doc['name'], custom_attributes=Properties(doc.get('custom_attributes', {}))
        )

    def build_link(self, doc) -> Link:
        return Link(
            type=doc['named_type'],
            targets=doc['targets'],
            is_toplevel=doc['is_toplevel'],
            custom_attributes=Properties(doc.get('custom_attributes', {})),
        )
