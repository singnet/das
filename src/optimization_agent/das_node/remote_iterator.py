from hyperon_das_node import MessageBrokerType
from das_node.query_answer import QueryAnswer
from das_node.query_element import QueryElement
from das_node.query_node import QueryNodeServer


class RemoteIterator(QueryElement):
    def __init__(self, local_id: str):
        super().__init__()
        self.local_id = local_id
        self.remote_input_buffer = None
        self._closed = False
        self.setup_buffers()

    def __del__(self):
        self.graceful_shutdown()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.graceful_shutdown()

    def graceful_shutdown(self):
        if not self._closed:
            if self.remote_input_buffer:
                self.remote_input_buffer.graceful_shutdown()
            self._closed = True

    def setup_buffers(self):
        if not self._closed:
            self.remote_input_buffer = QueryNodeServer(
                self.local_id, 
                MessageBrokerType.GRPC
            )

    def finished(self) -> bool:
        if self._closed or not self.remote_input_buffer:
            return True

        return (self.remote_input_buffer.is_query_answers_finished() and 
                self.remote_input_buffer.is_query_answers_empty())
    
    def pop(self) -> QueryAnswer:
        if self._closed or not self.remote_input_buffer:
            return None

        return self.remote_input_buffer.pop_query_answer()
