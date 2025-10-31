import argparse
import sys
import time

from hyperon_das.service_clients import PatternMatchingQueryProxy
from hyperon_das.service_clients.base import BaseQueryProxy
from hyperon_das.service_bus.service_bus import ServiceBusSingleton
from hyperon_das.commons.helpers import tokenize_preserve_quotes, str_2_bool


def parse_arguments():
    parser = argparse.ArgumentParser(description='Python query client')

    parser.add_argument("--client-id", required=True, help="<IP:PORT>")
    parser.add_argument("--server-id", required=True, help="<IP:PORT>")
    parser.add_argument("--update-attention-broker", required=False, type=str_2_bool, help="true/yes/1 or false/no/0", default=False)
    parser.add_argument("--positive-importance", required=False, type=str_2_bool, help="true/yes/1 or false/no/0", default=False)
    parser.add_argument("--populate-metta-mapping", required=False, type=str_2_bool, help="true/yes/1 or false/no/0", default=False)
    parser.add_argument("--max-query-answers", required=False, default=1)
    parser.add_argument("--query-tokens", required=True, type=str)

    return parser.parse_args()


def pattern_matching_query(
    client_id: str = None,
    server_id: str = None,
    update_attention_broker: bool = None,
    positive_importance: bool = None,
    populate_metta_mapping: bool = None,
    max_query_answers: int = None,
    query_tokens: list[str] | str = None
):
    if any([client_id, server_id, update_attention_broker, positive_importance, populate_metta_mapping, max_query_answers, query_tokens]):
        if not client_id:
            raise ValueError("client_id is required")

        if not server_id:
            raise ValueError("server_id is required")

        if not query_tokens:
            raise ValueError("query_tokens is required")

        if not update_attention_broker:
            update_attention_broker = False

        if not positive_importance:
            positive_importance = False

        if not populate_metta_mapping:
            populate_metta_mapping = False

        max_query_answers = int(max_query_answers) or 1

        if isinstance(query_tokens, str):
            query_tokens = tokenize_preserve_quotes(query_tokens)
    else:
        args = parse_arguments()

        if int(args.max_query_answers) <= 0:
            sys.exit("--max-query-answers cannot be 0 or negative")

        client_id = args.client_id
        server_id = args.server_id
        update_attention_broker = args.update_attention_broker
        positive_importance = args.positive_importance
        populate_metta_mapping = args.populate_metta_mapping
        max_query_answers = int(args.max_query_answers)
        query_tokens = tokenize_preserve_quotes(args.query_tokens)

    proxy = PatternMatchingQueryProxy(query_tokens, "")
    proxy.set_parameter(BaseQueryProxy.ATTENTION_UPDATE_FLAG, update_attention_broker)
    proxy.set_parameter(PatternMatchingQueryProxy.POSITIVE_IMPORTANCE_FLAG, positive_importance)
    proxy.set_parameter(BaseQueryProxy.POPULATE_METTA_MAPPING, populate_metta_mapping)
    proxy.set_parameter(BaseQueryProxy.MAX_ANSWERS, max_query_answers)

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
                print(f"query_answer: {answer.to_string()}")
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
