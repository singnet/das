import argparse

from distributed_optimizer import LeaderNode, WorkerNode


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Distributed Query Optimizer: Optimize query tokens using the specified configuration file. "
        )
    )
    parser.add_argument(
        '--config-file',
        required=True,
        default='config.cfg',
        help="Path to the configuration file (default: 'config.cfg')"
    )
    parser.add_argument(
        '--query-tokens',
        required=True,
        help="Query tokens to be optimized"
    )

    return parser.parse_args()


def main():
    try:
        args = parse_args()

        leader = LeaderNode(
            node_id="localhost:47000",
            attention_broker_server_id="localhost:37007",
            query_tokens=args.query_tokens,
            config_file=args.config_file
        )

        workers = []  # List to store worker references

        for i in range(1, int(leader.number_nodes)):
            worker = WorkerNode(
                node_id=f"localhost:4700{i}",
                server_id="localhost:47000",
                query_agent_node_id=f"localhost:3170{i}",
                query_agent_server_id="localhost:31700",
                config_file=args.config_file
            )
            workers.append(worker)

        leader.optimize_query()

        answers = leader.get_best_individuals("localhost:41705", "localhost:31700")

        return [answer.to_string() for answer in answers]
    except Exception as e:
        raise e


if __name__ == '__main__':
    print("Starting optimizer")
    print(main())
