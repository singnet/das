import argparse

from evolution.optimizer import QueryOptimizerAgent


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Query Optimizer: Optimize query tokens using the specified configuration file. "
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

        agent = QueryOptimizerAgent(config_file=args.config_file)
        iterator = agent.optimize(query_tokens=args.query_tokens)
        answers = [qa.to_string() for qa in iterator]
        print("\nResults:")
        return answers
    except Exception as e:
        raise e


if __name__ == '__main__':
    print("Starting optimizer")
    print(main())
