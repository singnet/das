from time import sleep
from typing import Any, Dict, Optional, Set

import grpc

import hyperon_das.grpc.common_pb2 as grpc_types
from hyperon_das.grpc.attention_broker_pb2_grpc import AttentionBrokerStub
from hyperon_das.logger import logger
from hyperon_das.utils import das_error


class AttentionBrokerGateway:
    def __init__(self, system_parameters: Dict[str, Any]):
        self.server_hostname = system_parameters.get("attention_broker_hostname")
        self.server_port = system_parameters.get("attention_broker_port")
        if self.server_hostname is None or self.server_port is None:
            das_error(
                ValueError(
                    f"Invalid system parameters. server_hostname: '{self.server_hostname}' server_port: {self.server_port}"
                )
            )
        self.server_url = f'{self.server_hostname}:{self.server_port}'
        self.ping()

    def ping(self) -> Optional[str]:
        logger().info(f'Pinging AttentionBroker at {self.server_url}')
        with grpc.insecure_channel(self.server_url) as channel:
            stub = AttentionBrokerStub(channel)
            response = stub.ping(grpc_types.Empty())
            logger().info(response.msg)
            return response.msg
        return None

    def stimulate(self, handle_count: Set[str]) -> Optional[str]:
        if handle_count is None:
            das_error(ValueError(f'Invalid handle_count {handle_count}'))
        logger().info(
            f'Requesting AttentionBroker at {self.server_url} to stimulate {len(handle_count)} atoms'
        )
        message = grpc_types.HandleCount(handle_count=handle_count)
        with grpc.insecure_channel(self.server_url) as channel:
            stub = AttentionBrokerStub(channel)
            response = stub.stimulate(message)
            logger().info(response.msg)
            return response.msg
        return None

    def correlate(self, handle_set: Set[str]) -> Optional[str]:
        if handle_set is None:
            das_error(ValueError(f'Invalid handle_set {handle_set}'))
        logger().info(
            f'Requesting AttentionBroker at {self.server_url} to correlate {len(handle_set)} atoms'
        )
        message = grpc_types.HandleList(handle_list=handle_set)
        sleep(0.05)
        with grpc.insecure_channel(self.server_url) as channel:
            stub = AttentionBrokerStub(channel)
            response = stub.correlate(message)
            logger().info(response.msg)
            return response.msg
        return None
