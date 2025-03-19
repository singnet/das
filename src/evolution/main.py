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
    # parser.add_argument(
    #     '--query-tokens',
    #     required=False,
    #     help="Query tokens to be optimized"
    # )

    return parser.parse_args()


def main():
    try:
        args = parse_args()
        agent = QueryOptimizerAgent(config_file=args.config_file)
        agent.run_server()
    except Exception as e:
        raise e


if __name__ == '__main__':
    def _sys():
        import sys
        sys.argv.extend([
            '--config-file',
            '/tmp/config.cfg'])
    _sys()
    print(main())
