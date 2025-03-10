# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""

import grpc

import hyperon_das.grpc.common_pb2 as common__pb2


class AttentionBrokerStub(object):
    """Missing associated documentation comment in .proto file."""

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.ping = channel.unary_unary(
            "/das.AttentionBroker/ping",
            request_serializer=common__pb2.Empty.SerializeToString,
            response_deserializer=common__pb2.Ack.FromString,
        )
        self.stimulate = channel.unary_unary(
            "/das.AttentionBroker/stimulate",
            request_serializer=common__pb2.HandleCount.SerializeToString,
            response_deserializer=common__pb2.Ack.FromString,
        )
        self.correlate = channel.unary_unary(
            "/das.AttentionBroker/correlate",
            request_serializer=common__pb2.HandleList.SerializeToString,
            response_deserializer=common__pb2.Ack.FromString,
        )


class AttentionBrokerServicer(object):
    """Missing associated documentation comment in .proto file."""

    def ping(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details("Method not implemented!")
        raise NotImplementedError("Method not implemented!")

    def stimulate(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details("Method not implemented!")
        raise NotImplementedError("Method not implemented!")

    def correlate(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details("Method not implemented!")
        raise NotImplementedError("Method not implemented!")


def add_AttentionBrokerServicer_to_server(servicer, server):
    rpc_method_handlers = {
        "ping": grpc.unary_unary_rpc_method_handler(
            servicer.ping,
            request_deserializer=common__pb2.Empty.FromString,
            response_serializer=common__pb2.Ack.SerializeToString,
        ),
        "stimulate": grpc.unary_unary_rpc_method_handler(
            servicer.stimulate,
            request_deserializer=common__pb2.HandleCount.FromString,
            response_serializer=common__pb2.Ack.SerializeToString,
        ),
        "correlate": grpc.unary_unary_rpc_method_handler(
            servicer.correlate,
            request_deserializer=common__pb2.HandleList.FromString,
            response_serializer=common__pb2.Ack.SerializeToString,
        ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
        "das.AttentionBroker", rpc_method_handlers
    )
    server.add_generic_rpc_handlers((generic_handler,))


# This class is part of an EXPERIMENTAL API.
class AttentionBroker(object):
    """Missing associated documentation comment in .proto file."""

    @staticmethod
    def ping(
        request,
        target,
        options=(),
        channel_credentials=None,
        call_credentials=None,
        insecure=False,
        compression=None,
        wait_for_ready=None,
        timeout=None,
        metadata=None,
    ):
        return grpc.experimental.unary_unary(
            request,
            target,
            "/das.AttentionBroker/ping",
            common__pb2.Empty.SerializeToString,
            common__pb2.Ack.FromString,
            options,
            channel_credentials,
            insecure,
            call_credentials,
            compression,
            wait_for_ready,
            timeout,
            metadata,
        )

    @staticmethod
    def stimulate(
        request,
        target,
        options=(),
        channel_credentials=None,
        call_credentials=None,
        insecure=False,
        compression=None,
        wait_for_ready=None,
        timeout=None,
        metadata=None,
    ):
        return grpc.experimental.unary_unary(
            request,
            target,
            "/das.AttentionBroker/stimulate",
            common__pb2.HandleCount.SerializeToString,
            common__pb2.Ack.FromString,
            options,
            channel_credentials,
            insecure,
            call_credentials,
            compression,
            wait_for_ready,
            timeout,
            metadata,
        )

    @staticmethod
    def correlate(
        request,
        target,
        options=(),
        channel_credentials=None,
        call_credentials=None,
        insecure=False,
        compression=None,
        wait_for_ready=None,
        timeout=None,
        metadata=None,
    ):
        return grpc.experimental.unary_unary(
            request,
            target,
            "/das.AttentionBroker/correlate",
            common__pb2.HandleList.SerializeToString,
            common__pb2.Ack.FromString,
            options,
            channel_credentials,
            insecure,
            call_credentials,
            compression,
            wait_for_ready,
            timeout,
            metadata,
        )
