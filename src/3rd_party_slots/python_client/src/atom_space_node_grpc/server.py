from concurrent import futures
import grpc
import atom_space_node_pb2_grpc


def server():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    atom_space_node_pb2_grpc.add_AtomSpaceNodeServicer_to_server(atom_space_node_pb2_grpc.AtomSpaceNodeServicer(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    server.wait_for_termination()


def serve():
    proxy = PatternMatchingQueryProxy()
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    atom_space_node_pb2_grpc.add_AtomSpaceNodeServicer_to_server(StarNode("127.0.0.1:50051", proxy), server)
    server.add_insecure_port("[::]:50051")
    print("gRPC server running...")
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    server()
