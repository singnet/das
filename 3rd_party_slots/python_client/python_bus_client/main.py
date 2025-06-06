import argparse
import sys
import time

from python_bus_client.proxy import PatternMatchingQueryHandler
from python_bus_client.service_bus import ServiceBusSingleton
from python_bus_client.helpers import tokenize_preserve_quotes


def parse_arguments():
    parser = argparse.ArgumentParser(description='Python query client')

    parser.add_argument("--client-id", required=True, help="<IP:PORT>")
    parser.add_argument("--server-id", required=True, help="<IP:PORT>")
    parser.add_argument("--update-attention-broker", required=False, help="True/1 or False/0", default=False)
    parser.add_argument("--max-query-answers", required=False, default=1)
    parser.add_argument("--query-tokens", required=True, type=str)

    return parser.parse_args()


def pattern_matching_query(
    client_id: str = None,
    server_id: str = None,
    update_attention_broker: bool = None,
    max_query_answers: int = None,
    query_tokens: list[str] | str = None
):
    if any([client_id, server_id, update_attention_broker, max_query_answers, query_tokens]):
        if not client_id:
            raise ValueError("client_id is required")

        if not server_id:
            raise ValueError("server_id is required")

        if not query_tokens:
            raise ValueError("query_tokens is required")

        max_query_answers = max_query_answers or 1
    
        if isinstance(query_tokens, str):
            query_tokens = tokenize_preserve_quotes(query_tokens)
    else:
        args = parse_arguments()

        if int(args.max_query_answers) <= 0:
            sys.exit("--max-query-answers cannot be 0 or negative")

        client_id = args.client_id
        server_id = args.server_id
        update_attention_broker = args.update_attention_broker
        max_query_answers = args.max_query_answers
        query_tokens = tokenize_preserve_quotes(args.query_tokens)

    proxy = PatternMatchingQueryHandler(
        tokens=query_tokens,
        update_attention_broker=bool(update_attention_broker)
    )

    service_bus = ServiceBusSingleton(
        host_id=client_id,
        known_peer=server_id,
        port_lower=54000,
        port_upper=54500
    ).get_instance()

    try:
        service_bus.issue_bus_command(proxy)
        count = 0
        while count < int(max_query_answers) and not proxy.finished():
            answer = proxy.pop()
            if answer:
                print(f"query_answer: {answer}")
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
    pattern_matching_query()
