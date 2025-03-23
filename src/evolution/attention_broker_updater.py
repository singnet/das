import time
import threading
import queue
from abc import ABC, abstractmethod

import grpc

import hyperon_das.grpc.common_pb2 as common__pb2
from hyperon_das.grpc.attention_broker_pb2_grpc import AttentionBrokerStub


ATTENTION_BROKER_ADDRESS = "localhost:37007"
MAX_STIMULATE_COUNT = 1
MAX_CORRELATIONS_WITHOUT_STIMULATE = 1000


class QueryAnswerProcessor(ABC):
    @abstractmethod
    def process_answer(self, query_answer):
        pass

    @abstractmethod
    def query_answers_finished(self):
        pass

    @abstractmethod
    def graceful_shutdown(self):
        pass


class AttentionBrokerUpdater(QueryAnswerProcessor):
    def __init__(self, address: str, context: str = "", atomdb: str = None):
        self.atomdb = atomdb
        self.attention_broker_address = address
        self.context = context
        self.flow_finished = False
        self.answers_queue = queue.Queue()
        self.flow_finished_mutex = threading.Lock()
        self.queue_processor_thread = threading.Thread(target=self.queue_processor, daemon=True)
        self.queue_processor_thread.start()

    def __del__(self):
        self.graceful_shutdown()

    def process_answer(self, query_answer):
        self.answers_queue.put(query_answer)

    def query_answers_finished(self):
        self.set_flow_finished()

    def graceful_shutdown(self):
        self.set_flow_finished()
        if self.queue_processor_thread is not None:
            self.queue_processor_thread.join()
            self.queue_processor_thread = None

    def set_flow_finished(self):
        with self.flow_finished_mutex:
            self.flow_finished = True

    def is_flow_finished(self) -> bool:
        with self.flow_finished_mutex:
            return self.flow_finished

    def queue_processor(self):
        single_answer = set()
        joint_answer = {}
        execution_stack = []
        weight_sum = 0
        correlated_count = 0
        stimulated_count = 0

        while True:
            if self.is_flow_finished() and self.answers_queue.empty():
                break

            idle_flag = True

            try:
                handles_answer = self.answers_queue.get_nowait()
            except queue.Empty:
                handles_answer = None

            while handles_answer is not None:
                if stimulated_count == MAX_STIMULATE_COUNT:
                    try:
                        handles_answer = self.answers_queue.get_nowait()
                    except queue.Empty:
                        handles_answer = None
                    continue

                for i in range(handles_answer.handles_size):
                    execution_stack.append(str(handles_answer.handles[i]))

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
                    message = common__pb2.HandleList(list=list(single_answer), context=self.context)
                    response = stub.correlate(message)
                    if response.msg != "CORRELATE":
                        print("Failed GRPC command: AttentionBroker::correlate()")

                single_answer.clear()

                idle_flag = False
                correlated_count += 1

                if correlated_count == MAX_CORRELATIONS_WITHOUT_STIMULATE:
                    correlated_count = 0

                    handle_count = {}
                    weight_sum = sum(joint_answer.values())
                    for handle, count in joint_answer.items():
                        handle_count[handle] = count
                    handle_count["SUM"] = weight_sum

                    message = common__pb2.HandleCount(map=handle_count, context=self.context)

                    with grpc.insecure_channel(self.attention_broker_address) as channel:
                        stub = AttentionBrokerStub(channel)
                        response = stub.stimulate(message)
                        if response.msg != "STIMULATE":
                            print("Failed GRPC command: AttentionBroker::stimulate()")

                    stimulated_count += 1
                    joint_answer.clear()

                try:
                    handles_answer = self.answers_queue.get_nowait()
                except queue.Empty:
                    handles_answer = None

            if idle_flag:
                time.sleep(0.1)

        if correlated_count > 0:
            weight_sum = sum(joint_answer.values())

            handle_count = {}
            weight_sum = sum(joint_answer.values())
            for handle, count in joint_answer.items():
                handle_count[handle] = count
            handle_count["SUM"] = weight_sum

            message = common__pb2.HandleCount(map=handle_count, context=self.context)

            with grpc.insecure_channel(self.attention_broker_address) as channel:
                stub = AttentionBrokerStub(channel)
                response = stub.stimulate(message)
                if response.msg != "STIMULATE":
                    print("Failed GRPC command: AttentionBroker::stimulate()")

            stimulated_count += 1

        self.set_flow_finished()
