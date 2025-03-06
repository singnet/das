import argparse

from optimizer import QueryOptimizerAgent


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Query Optimizer: Optimize query tokens using the specified configuration file. "
        )
    )
    parser.add_argument(
        '--config-file',
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
        agent = QueryOptimizerAgent(args.config_file)
        iterator = agent.optimize(args.query_tokens)
        return [str(item) for item in iterator]
    except Exception as e:
        raise e


if __name__ == '__main__':
    print(main())
