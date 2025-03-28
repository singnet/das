import grpc

import hyperon_das.grpc.attention_broker_pb2 as attention__broker__pb2

from hyperon_das.grpc.attention_broker_pb2_grpc import AttentionBrokerStub
from hyperon_das_atomdb.database import AtomDB

from hyperon_das_node import QueryAnswer


MAX_CORRELATIONS_WITHOUT_STIMULATE = 1000
MAX_STIMULATE_COUNT = 1


class AttentionBrokerUpdater:
    def __init__(self, address: str, atomdb: AtomDB, context: str = ""):
        self.attention_broker_address = address
        self.atomdb = atomdb
        self.context = context

    def update(self, individuals: list[tuple[QueryAnswer, float]]):
        single_answer = set()
        joint_answer = {}
        execution_stack = []
        weight_sum = 0
        correlated_count = 0
        stimulated_count = 0

        for query_answer, _ in individuals:
            if stimulated_count == MAX_STIMULATE_COUNT:
                continue

            for i in range(query_answer.handles_size):
                execution_stack.append(str(query_answer.handles[i]))

            while execution_stack:
                handle = execution_stack.pop()
                single_answer.add(handle)
                count = joint_answer.get(handle, 0) + 1
                joint_answer[handle] = count

                try:
                    targets = self.atomdb.get_link_targets(handle)
                    for target in targets:
                        execution_stack.append(target)
                except Exception:
                    pass

            with grpc.insecure_channel(self.attention_broker_address) as channel:
                stub = AttentionBrokerStub(channel)
                message = attention__broker__pb2.HandleList(list=list(single_answer), context=self.context)
                response = stub.correlate(message)
                if response.msg != "CORRELATE":
                    print("Failed GRPC command: AttentionBroker.correlate()")

            single_answer.clear()
            correlated_count += 1

            if correlated_count == MAX_CORRELATIONS_WITHOUT_STIMULATE:
                correlated_count = 0

                handle_count = {}
                weight_sum = sum(joint_answer.values())
                for handle, count in joint_answer.items():
                    handle_count[handle] = count
                handle_count["SUM"] = weight_sum

                message = attention__broker__pb2.HandleCount(map=handle_count, context=self.context)

                with grpc.insecure_channel(self.attention_broker_address) as channel:
                    stub = AttentionBrokerStub(channel)
                    response = stub.stimulate(message)
                    if response.msg != "STIMULATE":
                        print("Failed GRPC command: AttentionBroker.stimulate()")

                joint_answer.clear()
                stimulated_count += 1

        if correlated_count > 0:
            weight_sum = sum(joint_answer.values())

            handle_count = {}
            weight_sum = sum(joint_answer.values())
            for handle, count in joint_answer.items():
                handle_count[handle] = count
            handle_count["SUM"] = weight_sum

            message = attention__broker__pb2.HandleCount(map=handle_count, context=self.context)

            with grpc.insecure_channel(self.attention_broker_address) as channel:
                stub = AttentionBrokerStub(channel)
                response = stub.stimulate(message)
                if response.msg != "STIMULATE":
                    print("Failed GRPC command: AttentionBroker.stimulate()")

            stimulated_count += 1
