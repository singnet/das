#include "inference_iterator.h"
#include "Message.h"

using namespace inference_agent;
using namespace distributed_algorithm_node;
using namespace std;



template <class NodeType>
InferenceIterator<NodeType>::InferenceIterator(const string& local_id) {
    this->local_id = local_id;
    this->setup_buffers();
}

template <class NodeType>
InferenceIterator<NodeType>::~InferenceIterator() {
    this->graceful_shutdown();
}

template <class NodeType>
void InferenceIterator<NodeType>::setup_buffers() {
    this->remote_input_buffer = make_shared<NodeType>(this->local_id, MessageBrokerType::GRPC);
}   

template <class NodeType>
void InferenceIterator<NodeType>::graceful_shutdown() {
    this->remote_input_buffer->graceful_shutdown();
}

template <class NodeType>
bool InferenceIterator<NodeType>::finished() {
    return this->remote_input_buffer->is_answers_finished() &&
           this->remote_input_buffer->is_answers_empty();
}

template <class NodeType>
vector<string> InferenceIterator<NodeType>::pop() {
    return this->remote_input_buffer->pop_answer();
}

template <class NodeType>
string InferenceIterator<NodeType>::get_local_id() {
    return this->local_id;
}

