from hyperon_das_atomdb.adapters import RedisMongoDB
from hyperon_das_atomdb.database import LinkT, NodeT
from tokenizers.dict_query_tokenizer import DictQueryTokenizer
from das_agent_node import DASAgentNode
from collections import abc
import argparse
import asyncio
import json

async def get_request(node: DASAgentNode):
    request = node.pop_request()
    return request

def build_link(request) -> LinkT:
    link = DictQueryTokenizer.untokenize(request)
    tt = [dict(named_type=t["type"], **t) for t in link["targets"]]
    targets = [LinkT(**t) if t["atom_type"] == "link" else NodeT(**{"type":t["type"], "name":t["name"]}) for t in tt]
    custom_attributes = {}
    for k, v in link["custom_attributes"].items():
        custom_attributes.update(v)
    return LinkT(**{"type":link["type"], "targets": targets, "custom_attributes": custom_attributes})

def create_link(request, atom_db):
    link = build_link(" ".join(request))
    atom_db.add_link(link)
    atom_db.commit()
    print("Link created")

async def run(node: DASAgentNode, atom_db: RedisMongoDB):
    while True:
        request = await get_request(node)
        if request:
            create_link(request, atom_db)
        else:
            print("No request")
            await asyncio.sleep(1)

def load_config(path):
    with open(path, "r") as f:
        return json.load(f)

def main():
    parser = argparse.ArgumentParser(description="DAS Agent")
    parser.add_argument("--node_id", type=str, help="Node ID")
    parser.add_argument("--server_id", type=str, help="Server ID")
    parser.add_argument("--mongo_hostname", type=str, default="localhost", help="MongoDB hostname")
    parser.add_argument("--mongo_port", type=int, default=27017, help="MongoDB port")
    parser.add_argument("--mongo_username", type=str, default="mongo", help="MongoDB username")
    parser.add_argument("--mongo_password", type=str, default="mongo", help="MongoDB password")
    parser.add_argument("--mongo_tls_ca_file", type=str, default=None, help="MongoDB TLS CA file")
    parser.add_argument("--redis_hostname", type=str, default="localhost", help="Redis hostname")
    parser.add_argument("--redis_port", type=int, default=6379, help="Redis port")
    parser.add_argument("--redis_username", type=str, default=None, help="Redis username")
    parser.add_argument("--redis_password", type=str, default=None, help="Redis password")
    parser.add_argument("--redis_cluster", type=bool, default=True, help="Redis cluster")
    parser.add_argument("--redis_ssl", type=bool, default=True, help="Redis SSL")
    parser.add_argument("--config", type=str, default=None, help="Path to config file")
    args = parser.parse_args()

    if args.config:
        config = load_config(args.config)
        args = argparse.Namespace(**{**vars(args), **config})

    node = DASAgentNode(args.node_id, args.server_id)
    atom_db = RedisMongoDB(**{
        "mongo_hostname": args.mongo_hostname,
        "mongo_port": args.mongo_port,
        "mongo_username": args.mongo_username,
        "mongo_password": args.mongo_password,
        "mongo_tls_ca_file": args.mongo_tls_ca_file,
        "redis_hostname": args.redis_hostname,
        "redis_port": args.redis_port,
        "redis_username": args.redis_username,
        "redis_password": args.redis_password,
        "redis_cluster": args.redis_cluster,
        "redis_ssl": args.redis_ssl
    })

    asyncio.run(run(node, atom_db))

    


    




if __name__ == "__main__":
    main()

