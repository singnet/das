import argparse
import sys
import time

from src.proxy import PatternMatchingQueryProxy
from src.servicebus import ServiceBusSingleton


def preserve_quotes_parser(s: str):
    tokens = []
    current = []
    inside_quotes = False

    i = 0
    while i < len(s):
        c = s[i]

        if c == '"':
            if inside_quotes:
                current.append(c)
                tokens.append(''.join(current))
                current = []
                inside_quotes = False
            else:
                if current:
                    tokens.append(''.join(current))
                    current = []
                current.append(c)
                inside_quotes = True
        elif c == ' ' and not inside_quotes:
            if current:
                tokens.append(''.join(current))
                current = []
        else:
            current.append(c)

        i += 1

    if current:
        tokens.append(''.join(current))

    return tokens


def parse_arguments():
    parser = argparse.ArgumentParser(description='Python query client')

    parser.add_argument("--client-id", required=True, help="<IP:PORT>")
    parser.add_argument("--server-id", required=True, help="<IP:PORT>")
    parser.add_argument("--update-attention-broker", required=False, help="True/1 or False/0", default=False)
    parser.add_argument("--max-query-answers", required=False, default=1)
    parser.add_argument("--query-tokens", required=True, type=str)

    return parser.parse_args()


def main():
    args = parse_arguments()

    if int(args.max_query_answers) <= 0:
        sys.exit("--max-query-answers cannot be 0 or negative")

    tokens = preserve_quotes_parser(args.query_tokens)

    proxy = PatternMatchingQueryProxy(
        tokens=tokens,
        update_attention_broker=bool(args.update_attention_broker)
    )

    service_bus = ServiceBusSingleton(
        host_id=args.client_id,
        known_peer=args.server_id,
        port_lower=54000,
        port_upper=54500
    ).get_instance()

    try:
        service_bus.issue_bus_command(proxy)
        count = 0
        while count < int(args.max_query_answers) and not proxy.finished():
            answer = proxy.pop()
            if answer:
                print(f"\nquery_answer: {answer}\n")
                count += 1
            else:
                time.sleep(0.1)

        if count == 0:
            print("No match for query")
    except KeyboardInterrupt:
        pass
    finally:
        proxy.graceful_shutdown()


if __name__ == '__main__':
    main()
