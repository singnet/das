from hyperon_das_atomdb.adapters import RedisMongoDB
from hyperon_das_atomdb.database import LinkT, NodeT
from tokenizers.dict_query_tokenizer import DictQueryTokenizer
from das_agent_node import DASAgentNode
import argparse
import json
import time
import sys


def get_request(node: DASAgentNode):
    request = node.pop_request()
    return request


def parse_targets(targets, atom_db):
    new_targets = []
    for target in targets:
        if target.get("atom_type") == "link":
            new_targets.append(LinkT(**target))
        elif target.get("atom_type") == "node":
            new_targets.append(NodeT(**{"type": target["type"], "name": target["name"]}))
        elif target.get("handle"):
            new_targets.append(atom_db.get_atom(target["handle"]))
        else:
            print(target)
            raise Exception("Invalid target")
    return new_targets


def build_link(request, atom_db) -> LinkT:
    print("Building link")
    print(request)
    link = DictQueryTokenizer.untokenize(request)
    print(link)
    tt = [dict(named_type=t.get("type"), **t) for t in link["targets"]]
    targets = parse_targets(tt, atom_db)
    custom_attributes = {}
    for k, v in link["custom_attributes"].items():
        custom_attributes.update(v)
    return LinkT(
        **{
            "type": link["type"],
            "targets": targets,
            "custom_attributes": custom_attributes,
        }
    )


def create_link(request, atom_db):
    link = build_link(" ".join(request), atom_db)
    atom_db.add_link(link)
    atom_db.commit()
    print("Link created")


def run(node: DASAgentNode, atom_db: RedisMongoDB):
    while True:
        print("Processing request")
        sys.stdout.flush()
        request = get_request(node)
        sys.stdout.flush()
        create_link(request, atom_db)
        sys.stdout.flush()
        time.sleep(1)


def load_config(path):
    with open(path, "r") as f:
        return json.load(f)


def main():
    parser = argparse.ArgumentParser(description="DAS Agent")
    parser.add_argument("--node_id", type=str, help="Node ID")
    parser.add_argument("--mongo_hostname", type=str, default="localhost", help="MongoDB hostname")
    parser.add_argument("--mongo_port", type=int, default=27017, help="MongoDB port")
    parser.add_argument("--mongo_username", type=str, default="mongo", help="MongoDB username")
    parser.add_argument("--mongo_password", type=str, default="mongo", help="MongoDB password")
    parser.add_argument("--mongo_tls_ca_file", type=str, default=None, help="MongoDB TLS CA file")
    parser.add_argument("--redis_hostname", type=str, default="localhost", help="Redis hostname")
    parser.add_argument("--redis_port", type=int, default=6379, help="Redis port")
    parser.add_argument("--redis_username", type=str, default=None, help="Redis username")
    parser.add_argument("--redis_password", type=str, default=None, help="Redis password")
    parser.add_argument("--redis_cluster", type=bool, default=False, help="Redis cluster")
    parser.add_argument("--redis_ssl", type=bool, default=False, help="Redis SSL")
    parser.add_argument("--config", type=str, default=None, help="Json config")
    parser.add_argument("--config_file", type=str, default=None, help="Path to config file")

    _args = parser.parse_args()

    if _args.config:
        config = json.loads(_args.config)
        _args = argparse.Namespace(**{**vars(_args), **config})
    elif _args.config_file:
        config = load_config(_args.config_file)
        _args = argparse.Namespace(**{**vars(_args), **config})

    _node = DASAgentNode(_args.node_id)
    _atom_db = RedisMongoDB(
        **{
            "mongo_hostname": _args.mongo_hostname,
            "mongo_port": _args.mongo_port,
            "mongo_username": _args.mongo_username,
            "mongo_password": _args.mongo_password,
            "mongo_tls_ca_file": _args.mongo_tls_ca_file,
            "redis_hostname": _args.redis_hostname,
            "redis_port": _args.redis_port,
            "redis_username": _args.redis_username,
            "redis_password": _args.redis_password,
            "redis_cluster": _args.redis_cluster,
            "redis_ssl": _args.redis_ssl,
        }
    )
    run(_node, _atom_db)


main()
