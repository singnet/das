import signal
import sys
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
    return parser.parse_args()


def shutdown_handler(signum, frame):
    print("Stopping server...")
    sys.exit(0)


def main():
    try:
        args = parse_args()
        agent = QueryOptimizerAgent(config_file=args.config_file)

        signal.signal(signal.SIGINT, shutdown_handler)
        signal.signal(signal.SIGTERM, shutdown_handler)

        agent.run_server()
    except Exception as e:
        print(f"An error occurred - Details: {str(e)}")
        return 1


if __name__ == '__main__':
    sys.exit(main())
